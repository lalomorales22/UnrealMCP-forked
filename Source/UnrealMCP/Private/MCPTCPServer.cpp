#include "MCPTCPServer.h"
#include "Engine/World.h"
#include "Editor.h"
#include "LevelEditor.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "JsonObjectConverter.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "ActorEditorUtils.h"
#include "EngineUtils.h"
#include "Containers/Ticker.h"
#include "UnrealMCP.h"
#include "MCPFileLogger.h"

// Shorthand for logger
#define MCP_LOG(Verbosity, Format, ...) FMCPFileLogger::Get().Log(ELogVerbosity::Verbosity, FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_INFO(Format, ...) FMCPFileLogger::Get().Info(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_ERROR(Format, ...) FMCPFileLogger::Get().Error(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_WARNING(Format, ...) FMCPFileLogger::Get().Warning(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_VERBOSE(Format, ...) FMCPFileLogger::Get().Verbose(FString::Printf(TEXT(Format), ##__VA_ARGS__))

FMCPTCPServer::FMCPTCPServer(int32 InPort) 
    : Port(InPort)
    , Listener(nullptr)
    , bRunning(false)
    , ClientTimeoutSeconds(30.0f)
{
    // Initialize command handlers with the new signature that includes the client socket
    CommandHandlers.Add("get_scene_info", [this](const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) { HandleGetSceneInfo(Params, ClientSocket); });
    CommandHandlers.Add("create_object", [this](const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) { HandleCreateObject(Params, ClientSocket); });
    CommandHandlers.Add("modify_object", [this](const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) { HandleModifyObject(Params, ClientSocket); });
    CommandHandlers.Add("delete_object", [this](const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) { HandleDeleteObject(Params, ClientSocket); });
}

FMCPTCPServer::~FMCPTCPServer()
{
    Stop();
}

bool FMCPTCPServer::Start()
{
    if (bRunning)
    {
        MCP_LOG_WARNING("Start called but server is already running, returning true");
        return true;
    }
    
    MCP_LOG_WARNING("Starting MCP server on port %d", Port);
    
    // Use a simple ASCII string for the socket description to avoid encoding issues
    Listener = new FTcpListener(FIPv4Endpoint(FIPv4Address::Any, Port));
    if (!Listener || !Listener->IsActive())
    {
        MCP_LOG_ERROR("Failed to start MCP server on port %d", Port);
        Stop();
        return false;
    }

    // Clear any existing client connections
    ClientConnections.Empty();

    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FMCPTCPServer::Tick), 0.1f);
    bRunning = true;
    MCP_LOG_INFO("MCP Server started on port %d", Port);
    return true;
}

void FMCPTCPServer::Stop()
{
    // Clean up all client connections
    CleanupAllClientConnections();
    
    if (Listener)
    {
        delete Listener;
        Listener = nullptr;
    }
    
    if (TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
        TickerHandle.Reset();
    }
    
    bRunning = false;
    MCP_LOG_INFO("MCP Server stopped");
}

bool FMCPTCPServer::Tick(float DeltaTime)
{
    if (!bRunning) return false;
    
    // Normal processing
    ProcessPendingConnections();
    ProcessClientData();
    CheckClientTimeouts(DeltaTime);
    return true;
}

void FMCPTCPServer::ProcessPendingConnections()
{
    if (!Listener) return;
    
    // Always accept new connections
    if (!Listener->OnConnectionAccepted().IsBound())
    {
        Listener->OnConnectionAccepted().BindRaw(this, &FMCPTCPServer::HandleConnectionAccepted);
    }
}

bool FMCPTCPServer::HandleConnectionAccepted(FSocket* InSocket, const FIPv4Endpoint& Endpoint)
{
    if (!InSocket)
    {
        MCP_LOG_ERROR("HandleConnectionAccepted called with null socket");
        return false;
    }

    MCP_LOG_VERBOSE("Connection attempt from %s", *Endpoint.ToString());
    
    // Accept all connections
    InSocket->SetNonBlocking(true);
    
    // Add to our list of client connections
    ClientConnections.Add(FMCPClientConnection(InSocket, Endpoint));
    
    MCP_LOG_INFO("MCP Client connected from %s (Total clients: %d)", *Endpoint.ToString(), ClientConnections.Num());
    return true;
}

void FMCPTCPServer::ProcessClientData()
{
    // Make a copy of the array since we might modify it during iteration
    TArray<FMCPClientConnection> ConnectionsCopy = ClientConnections;
    
    for (FMCPClientConnection& ClientConnection : ConnectionsCopy)
    {
        if (!ClientConnection.Socket) continue;
        
        // Check if the client is still connected
        uint32 PendingDataSize = 0;
        if (!ClientConnection.Socket->HasPendingData(PendingDataSize))
        {
            // Try to check connection status
            uint8 DummyBuffer[1];
            int32 BytesRead = 0;
            
            bool bConnectionLost = false;
            
            try
            {
                if (!ClientConnection.Socket->Recv(DummyBuffer, 1, BytesRead, ESocketReceiveFlags::Peek))
                {
                    // Check if it's a real error or just a non-blocking socket that would block
                    int32 ErrorCode = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
                    if (ErrorCode != SE_EWOULDBLOCK)
                    {
                        // Real connection error
                        MCP_LOG_INFO("Client connection from %s appears to be closed (error code %d), cleaning up", 
                            *ClientConnection.Endpoint.ToString(), ErrorCode);
                        bConnectionLost = true;
                    }
                }
            }
            catch (...)
            {
                MCP_LOG_ERROR("Exception while checking client connection status for %s", 
                    *ClientConnection.Endpoint.ToString());
                bConnectionLost = true;
            }
            
            if (bConnectionLost)
            {
                CleanupClientConnection(ClientConnection);
                continue; // Skip to the next client
            }
        }
        
        // Reset PendingDataSize and check again to ensure we have the latest value
        PendingDataSize = 0;
        if (ClientConnection.Socket->HasPendingData(PendingDataSize))
        {
            MCP_LOG_VERBOSE("Client from %s has %u bytes of pending data", 
                *ClientConnection.Endpoint.ToString(), PendingDataSize);
            
            // Reset timeout timer since we're receiving data
            ClientConnection.TimeSinceLastActivity = 0.0f;
            
            int32 BytesRead = 0;
            if (ClientConnection.Socket->Recv(ClientConnection.ReceiveBuffer.GetData(), ClientConnection.ReceiveBuffer.Num(), BytesRead))
            {
                if (BytesRead > 0)
                {
                    MCP_LOG_VERBOSE("Read %d bytes from client %s", BytesRead, *ClientConnection.Endpoint.ToString());
                    
                    // Null-terminate the buffer to ensure it's a valid string
                    ClientConnection.ReceiveBuffer[BytesRead] = 0;
                    FString ReceivedData = FString(UTF8_TO_TCHAR(ClientConnection.ReceiveBuffer.GetData()));
                    ProcessCommand(ReceivedData, ClientConnection.Socket);
                }
            }
            else
            {
                // Check if it's a real error or just a non-blocking socket that would block
                int32 ErrorCode = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
                if (ErrorCode != SE_EWOULDBLOCK)
                {
                    // Real connection error, close the socket
                    MCP_LOG_WARNING("Socket error %d for client %s, closing connection", 
                        ErrorCode, *ClientConnection.Endpoint.ToString());
                    CleanupClientConnection(ClientConnection);
                }
            }
        }
    }
}

void FMCPTCPServer::CheckClientTimeouts(float DeltaTime)
{
    // Make a copy of the array since we might modify it during iteration
    TArray<FMCPClientConnection> ConnectionsCopy = ClientConnections;
    
    for (FMCPClientConnection& ClientConnection : ConnectionsCopy)
    {
        if (!ClientConnection.Socket) continue;
        
        // Increment time since last activity
        ClientConnection.TimeSinceLastActivity += DeltaTime;
        
        // Check if client has timed out
        if (ClientConnection.TimeSinceLastActivity > ClientTimeoutSeconds)
        {
            MCP_LOG_WARNING("Client from %s timed out after %.1f seconds of inactivity, disconnecting", 
                *ClientConnection.Endpoint.ToString(), ClientConnection.TimeSinceLastActivity);
            CleanupClientConnection(ClientConnection);
        }
    }
}

void FMCPTCPServer::CleanupAllClientConnections()
{
    MCP_LOG_INFO("Cleaning up all client connections (%d total)", ClientConnections.Num());
    
    // Make a copy of the array since we'll be modifying it during iteration
    TArray<FMCPClientConnection> ConnectionsCopy = ClientConnections;
    
    for (FMCPClientConnection& Connection : ConnectionsCopy)
    {
        CleanupClientConnection(Connection);
    }
    
    // Ensure the array is empty
    ClientConnections.Empty();
}

void FMCPTCPServer::CleanupClientConnection(FSocket* ClientSocket)
{
    if (!ClientSocket) return;
    
    // Find the client connection with this socket
    for (FMCPClientConnection& Connection : ClientConnections)
    {
        if (Connection.Socket == ClientSocket)
        {
            CleanupClientConnection(Connection);
            break;
        }
    }
}

void FMCPTCPServer::CleanupClientConnection(FMCPClientConnection& ClientConnection)
{
    if (!ClientConnection.Socket) return;
    
    MCP_LOG_INFO("Cleaning up client connection from %s", *ClientConnection.Endpoint.ToString());
    
    try
    {
        // Get the socket description before closing
        FString SocketDesc = GetSafeSocketDescription(ClientConnection.Socket);
        MCP_LOG_VERBOSE("Closing client socket with description: %s", *SocketDesc);
        
        // First close the socket
        bool bCloseSuccess = ClientConnection.Socket->Close();
        if (!bCloseSuccess)
        {
            MCP_LOG_ERROR("Failed to close client socket");
        }
        
        // Then destroy it
        ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        if (SocketSubsystem)
        {
            SocketSubsystem->DestroySocket(ClientConnection.Socket);
            MCP_LOG_VERBOSE("Successfully destroyed client socket");
        }
        else
        {
            MCP_LOG_ERROR("Failed to get socket subsystem when cleaning up client connection");
        }
    }
    catch (const std::exception& Ex)
    {
        MCP_LOG_ERROR("Exception while cleaning up client connection: %s", UTF8_TO_TCHAR(Ex.what()));
    }
    catch (...)
    {
        MCP_LOG_ERROR("Unknown exception while cleaning up client connection");
    }
    
    // Remove from our list of connections
    ClientConnections.RemoveAll([&ClientConnection](const FMCPClientConnection& Connection) {
        return Connection.Socket == ClientConnection.Socket;
    });
    
    MCP_LOG_INFO("MCP Client disconnected (Remaining clients: %d)", ClientConnections.Num());
}

void FMCPTCPServer::ProcessCommand(const FString& CommandJson, FSocket* ClientSocket)
{
    MCP_LOG_VERBOSE("Processing command: %s", *CommandJson);
    
    TSharedPtr<FJsonObject> Command;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CommandJson);
    if (FJsonSerializer::Deserialize(Reader, Command) && Command.IsValid())
    {
        FString Type;
        if (Command->TryGetStringField(FStringView(TEXT("type")), Type) && CommandHandlers.Contains(Type))
        {
            MCP_LOG_INFO("Processing command: %s", *Type);
            
            const TSharedPtr<FJsonObject>* ParamsPtr = nullptr;
            if (Command->TryGetObjectField(FStringView(TEXT("params")), ParamsPtr) && ParamsPtr != nullptr)
            {
                CommandHandlers[Type](*ParamsPtr, ClientSocket);
            }
            else
            {
                // Empty params
                CommandHandlers[Type](MakeShared<FJsonObject>(), ClientSocket);
            }
        }
        else
        {
            MCP_LOG_WARNING("Unknown command: %s", *Type);
            
            TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
            Response->SetStringField("status", "error");
            Response->SetStringField("message", FString::Printf(TEXT("Unknown command: %s"), *Type));
            SendResponse(ClientSocket, Response);
        }
    }
    else
    {
        MCP_LOG_WARNING("Invalid JSON format: %s", *CommandJson);
        
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "error");
        Response->SetStringField("message", TEXT("Invalid JSON format"));
        SendResponse(ClientSocket, Response);
    }
    
    // Keep the connection open for future commands
    // Do not close the socket here
}

void FMCPTCPServer::SendResponse(FSocket* Client, const TSharedPtr<FJsonObject>& Response)
{
    if (!Client) return;
    
    FString ResponseStr;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseStr);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
    
    MCP_LOG_VERBOSE("Preparing to send response: %s", *ResponseStr);
    
    FTCHARToUTF8 Converter(*ResponseStr);
    int32 BytesSent = 0;
    int32 TotalBytes = Converter.Length();
    const uint8* Data = (const uint8*)Converter.Get();
    
    // Ensure all data is sent
    while (BytesSent < TotalBytes)
    {
        int32 SentThisTime = 0;
        if (!Client->Send(Data + BytesSent, TotalBytes - BytesSent, SentThisTime))
        {
            MCP_LOG_WARNING("Failed to send response");
            break;
        }
        
        if (SentThisTime <= 0)
        {
            // Would block, try again next tick
            MCP_LOG_VERBOSE("Socket would block, will try again next tick");
            break;
        }
        
        BytesSent += SentThisTime;
        MCP_LOG_VERBOSE("Sent %d/%d bytes", BytesSent, TotalBytes);
    }
    
    if (BytesSent == TotalBytes)
    {
        MCP_LOG_INFO("Successfully sent complete response (%d bytes)", TotalBytes);
    }
    else
    {
        MCP_LOG_WARNING("Only sent %d/%d bytes of response", BytesSent, TotalBytes);
    }
}

void FMCPTCPServer::HandleGetSceneInfo(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    MCP_LOG_INFO("Handling get_scene_info command");
    
    UWorld* World = GEditor->GetEditorWorldContext().World();
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> ActorsArray;

    int32 ActorCount = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        TSharedPtr<FJsonObject> ActorInfo = MakeShared<FJsonObject>();
        ActorInfo->SetStringField("name", Actor->GetName());
        ActorInfo->SetStringField("type", Actor->GetClass()->GetName());
        
        // Add location
        FVector Location = Actor->GetActorLocation();
        TArray<TSharedPtr<FJsonValue>> LocationArray;
        LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
        LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
        LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
        ActorInfo->SetArrayField("location", LocationArray);
        
        ActorsArray.Add(MakeShared<FJsonValueObject>(ActorInfo));
        ActorCount++;
        if (ActorCount >= 100) break; // Limit for performance
    }

    Result->SetStringField("level", World->GetName());
    Result->SetNumberField("actor_count", ActorCount);
    Result->SetArrayField("actors", ActorsArray);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField("status", "success");
    Response->SetObjectField("result", Result);
    
    MCP_LOG_INFO("Sending get_scene_info response with %d actors", ActorCount);
    
    SendResponse(ClientSocket, Response);
    
    MCP_LOG_INFO("get_scene_info response sent");
}

void FMCPTCPServer::HandleCreateObject(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    
    FString Type;
    if (!Params->TryGetStringField(FStringView(TEXT("type")), Type))
    {
        MCP_LOG_WARNING("Missing 'type' field in create_object command");
        Response->SetStringField("status", "error");
        Response->SetStringField("message", "Missing 'type' field");
        SendResponse(ClientSocket, Response);
        return;
    }
    
    if (Type == "StaticMeshActor")
    {
        // Get location
        const TArray<TSharedPtr<FJsonValue>>* LocationArrayPtr = nullptr;
        if (!Params->TryGetArrayField(FStringView(TEXT("location")), LocationArrayPtr) || !LocationArrayPtr || LocationArrayPtr->Num() != 3)
        {
            MCP_LOG_WARNING("Invalid 'location' field in create_object command");
            Response->SetStringField("status", "error");
            Response->SetStringField("message", "Invalid 'location' field");
            SendResponse(ClientSocket, Response);
            return;
        }
        
        FVector Location(
            (*LocationArrayPtr)[0]->AsNumber(),
            (*LocationArrayPtr)[1]->AsNumber(),
            (*LocationArrayPtr)[2]->AsNumber()
        );
        
        // Create the actor
        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = NAME_None; // Auto-generate a name
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        
        AStaticMeshActor* NewActor = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, SpawnParams);
        if (NewActor)
        {
            MCP_LOG_INFO("Created StaticMeshActor at location (%f, %f, %f)", Location.X, Location.Y, Location.Z);
            
            // Set mesh if specified
            FString MeshPath;
            if (Params->TryGetStringField(FStringView(TEXT("mesh")), MeshPath))
            {
                UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
                if (Mesh)
                {
                    NewActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
                    MCP_LOG_INFO("Set mesh to %s", *MeshPath);
                }
                else
                {
                    MCP_LOG_WARNING("Failed to load mesh %s", *MeshPath);
                }
            }
            
            Response->SetStringField("status", "success");
            Response->SetStringField("actor_name", NewActor->GetName());
        }
        else
        {
            MCP_LOG_ERROR("Failed to create StaticMeshActor");
            Response->SetStringField("status", "error");
            Response->SetStringField("message", "Failed to create actor");
        }
    }
    else
    {
        MCP_LOG_WARNING("Unsupported actor type: %s", *Type);
        Response->SetStringField("status", "error");
        Response->SetStringField("message", FString::Printf(TEXT("Unsupported actor type: %s"), *Type));
    }
    
    SendResponse(ClientSocket, Response);
}

void FMCPTCPServer::HandleModifyObject(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    
    FString ActorName;
    if (!Params->TryGetStringField(FStringView(TEXT("name")), ActorName))
    {
        MCP_LOG_WARNING("Missing 'name' field in modify_object command");
        Response->SetStringField("status", "error");
        Response->SetStringField("message", "Missing 'name' field");
        SendResponse(ClientSocket, Response);
        return;
    }
    
    AActor* Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }
    
    if (!Actor)
    {
        MCP_LOG_WARNING("Actor not found: %s", *ActorName);
        Response->SetStringField("status", "error");
        Response->SetStringField("message", FString::Printf(TEXT("Actor not found: %s"), *ActorName));
        SendResponse(ClientSocket, Response);
        return;
    }
    
    bool bModified = false;
    
    // Check for location update
    const TArray<TSharedPtr<FJsonValue>>* LocationArrayPtr = nullptr;
    if (Params->TryGetArrayField(FStringView(TEXT("location")), LocationArrayPtr) && LocationArrayPtr && LocationArrayPtr->Num() == 3)
    {
        FVector NewLocation(
            (*LocationArrayPtr)[0]->AsNumber(),
            (*LocationArrayPtr)[1]->AsNumber(),
            (*LocationArrayPtr)[2]->AsNumber()
        );
        
        Actor->SetActorLocation(NewLocation);
        MCP_LOG_INFO("Updated location of %s to (%f, %f, %f)", *ActorName, NewLocation.X, NewLocation.Y, NewLocation.Z);
        bModified = true;
    }
    
    // Check for rotation update
    const TArray<TSharedPtr<FJsonValue>>* RotationArrayPtr = nullptr;
    if (Params->TryGetArrayField(FStringView(TEXT("rotation")), RotationArrayPtr) && RotationArrayPtr && RotationArrayPtr->Num() == 3)
    {
        FRotator NewRotation(
            (*RotationArrayPtr)[0]->AsNumber(),
            (*RotationArrayPtr)[1]->AsNumber(),
            (*RotationArrayPtr)[2]->AsNumber()
        );
        
        Actor->SetActorRotation(NewRotation);
        MCP_LOG_INFO("Updated rotation of %s to (%f, %f, %f)", *ActorName, NewRotation.Pitch, NewRotation.Yaw, NewRotation.Roll);
        bModified = true;
    }
    
    // Check for scale update
    const TArray<TSharedPtr<FJsonValue>>* ScaleArrayPtr = nullptr;
    if (Params->TryGetArrayField(FStringView(TEXT("scale")), ScaleArrayPtr) && ScaleArrayPtr && ScaleArrayPtr->Num() == 3)
    {
        FVector NewScale(
            (*ScaleArrayPtr)[0]->AsNumber(),
            (*ScaleArrayPtr)[1]->AsNumber(),
            (*ScaleArrayPtr)[2]->AsNumber()
        );
        
        Actor->SetActorScale3D(NewScale);
        MCP_LOG_INFO("Updated scale of %s to (%f, %f, %f)", *ActorName, NewScale.X, NewScale.Y, NewScale.Z);
        bModified = true;
    }
    
    if (bModified)
    {
        Response->SetStringField("status", "success");
    }
    else
    {
        MCP_LOG_WARNING("No modifications specified for %s", *ActorName);
        Response->SetStringField("status", "warning");
        Response->SetStringField("message", "No modifications specified");
    }
    
    SendResponse(ClientSocket, Response);
}

void FMCPTCPServer::HandleDeleteObject(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    
    FString ActorName;
    if (!Params->TryGetStringField(FStringView(TEXT("name")), ActorName))
    {
        MCP_LOG_WARNING("Missing 'name' field in delete_object command");
        Response->SetStringField("status", "error");
        Response->SetStringField("message", "Missing 'name' field");
        SendResponse(ClientSocket, Response);
        return;
    }
    
    AActor* Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }
    
    if (!Actor)
    {
        MCP_LOG_WARNING("Actor not found: %s", *ActorName);
        Response->SetStringField("status", "error");
        Response->SetStringField("message", FString::Printf(TEXT("Actor not found: %s"), *ActorName));
        SendResponse(ClientSocket, Response);
        return;
    }
    
    // Check if the actor can be deleted
    if (!FActorEditorUtils::IsABuilderBrush(Actor))
    {
        bool bDestroyed = World->DestroyActor(Actor);
        if (bDestroyed)
        {
            MCP_LOG_INFO("Deleted actor: %s", *ActorName);
            Response->SetStringField("status", "success");
        }
        else
        {
            MCP_LOG_ERROR("Failed to delete actor: %s", *ActorName);
            Response->SetStringField("status", "error");
            Response->SetStringField("message", FString::Printf(TEXT("Failed to delete actor: %s"), *ActorName));
        }
    }
    else
    {
        MCP_LOG_WARNING("Cannot delete special actor: %s", *ActorName);
        Response->SetStringField("status", "error");
        Response->SetStringField("message", FString::Printf(TEXT("Cannot delete special actor: %s"), *ActorName));
    }
    
    SendResponse(ClientSocket, Response);
}

FString FMCPTCPServer::GetSafeSocketDescription(FSocket* Socket)
{
    if (!Socket)
    {
        return TEXT("NullSocket");
    }
    
    try
    {
        FString Description = Socket->GetDescription();
        
        // Check if the description contains any non-ASCII characters
        bool bHasNonAscii = false;
        for (TCHAR Char : Description)
        {
            if (Char > 127)
            {
                bHasNonAscii = true;
                break;
            }
        }
        
        if (bHasNonAscii)
        {
            // Return a safe description instead
            return TEXT("Socket_") + FString::FromInt(reinterpret_cast<uint64>(Socket));
        }
        
        return Description;
    }
    catch (...)
    {
        // If there's any exception, return a safe description
        return TEXT("Socket_") + FString::FromInt(reinterpret_cast<uint64>(Socket));
    }
} 
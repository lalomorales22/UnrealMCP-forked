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
    , ClientSocket(nullptr)
    , bRunning(false)
    , ClientTimeoutSeconds(30.0f)
    , TimeSinceLastClientActivity(0.0f)
{
    ReceiveBuffer.SetNumUninitialized(8192); // 8KB buffer
    
    CommandHandlers.Add("get_scene_info", [this](const TSharedPtr<FJsonObject>& Params) { HandleGetSceneInfo(Params); });
    CommandHandlers.Add("create_object", [this](const TSharedPtr<FJsonObject>& Params) { HandleCreateObject(Params); });
    CommandHandlers.Add("modify_object", [this](const TSharedPtr<FJsonObject>& Params) { HandleModifyObject(Params); });
    CommandHandlers.Add("delete_object", [this](const TSharedPtr<FJsonObject>& Params) { HandleDeleteObject(Params); });
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

    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FMCPTCPServer::Tick), 0.1f);
    bRunning = true;
    MCP_LOG_INFO("MCP Server started on port %d", Port);
    return true;
}

void FMCPTCPServer::Stop()
{
    if (ClientSocket)
    {
        CleanupClientConnection();
    }
    
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
    CheckClientTimeout(DeltaTime);
    return true;
}

void FMCPTCPServer::ProcessPendingConnections()
{
    if (!Listener) return;
    
    // Accept new connections if we don't have a client
    if (!ClientSocket)
    {
        // Set up a delegate to handle incoming connections
        if (!Listener->OnConnectionAccepted().IsBound())
        {
            Listener->OnConnectionAccepted().BindRaw(this, &FMCPTCPServer::HandleConnectionAccepted);
        }
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
    
    // Only accept one connection at a time
    if (ClientSocket != nullptr)
    {
        // Check if the existing client is still connected
        uint32 PendingDataSize = 0;
        uint8 DummyBuffer[1];
        int32 BytesRead = 0;
        
        bool bExistingClientConnected = true;
        
        // Try to peek at the socket to check if it's still connected
        if (!ClientSocket->HasPendingData(PendingDataSize) && 
            !ClientSocket->Recv(DummyBuffer, 1, BytesRead, ESocketReceiveFlags::Peek))
        {
            // Existing client appears to be disconnected
            MCP_LOG_WARNING("Existing client appears to be disconnected, cleaning up before accepting new connection");
            CleanupClientConnection();
            bExistingClientConnected = false;
        }
        
        if (bExistingClientConnected)
        {
            MCP_LOG_WARNING("Rejecting connection from %s - already have an active client", *Endpoint.ToString());
            
            // CRITICAL CHANGE: Simply return false to reject the connection
            // Let the TcpListener handle closing/destroying the socket
            // This avoids the crash when we try to close it ourselves
            return false;
        }
    }
    
    ClientSocket = InSocket;
    ClientSocket->SetNonBlocking(true);
    TimeSinceLastClientActivity = 0.0f;
    MCP_LOG_INFO("MCP Client connected from %s", *Endpoint.ToString());
    return true;
}

void FMCPTCPServer::ProcessClientData()
{
    if (!ClientSocket) return;
    
    // Check if the client is still connected
    uint32 PendingDataSize = 0;
    if (!ClientSocket->HasPendingData(PendingDataSize))
    {
        // Try to check connection status
        uint8 DummyBuffer[1];
        int32 BytesRead = 0;
        
        bool bConnectionLost = false;
        
        try
        {
            if (!ClientSocket->Recv(DummyBuffer, 1, BytesRead, ESocketReceiveFlags::Peek))
            {
                // Check if it's a real error or just a non-blocking socket that would block
                int32 ErrorCode = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
                if (ErrorCode != SE_EWOULDBLOCK)
                {
                    // Real connection error
                    MCP_LOG_INFO("Client connection appears to be closed (error code %d), cleaning up", ErrorCode);
                    bConnectionLost = true;
                }
            }
        }
        catch (...)
        {
            MCP_LOG_ERROR("Exception while checking client connection status");
            bConnectionLost = true;
        }
        
        if (bConnectionLost)
        {
            CleanupClientConnection();
            return;
        }
    }
    
    // Reset PendingDataSize and check again to ensure we have the latest value
    PendingDataSize = 0;
    if (ClientSocket->HasPendingData(PendingDataSize))
    {
        MCP_LOG_VERBOSE("Client has %u bytes of pending data", PendingDataSize);
        
        // Reset timeout timer since we're receiving data
        TimeSinceLastClientActivity = 0.0f;
        
        int32 BytesRead = 0;
        if (ClientSocket->Recv(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), BytesRead))
        {
            if (BytesRead > 0)
            {
                MCP_LOG_VERBOSE("Read %d bytes from client", BytesRead);
                
                FString ReceivedData = FString(UTF8_TO_TCHAR(ReceiveBuffer.GetData()));
                ProcessCommand(ReceivedData);
            }
        }
        else
        {
            // Check if it's a real error or just a non-blocking socket that would block
            int32 ErrorCode = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
            if (ErrorCode != SE_EWOULDBLOCK)
            {
                // Real connection error, close the socket
                MCP_LOG_WARNING("Socket error %d, closing connection", ErrorCode);
                CleanupClientConnection();
            }
        }
    }
}

void FMCPTCPServer::CheckClientTimeout(float DeltaTime)
{
    if (!ClientSocket) return;
    
    // Increment time since last activity
    TimeSinceLastClientActivity += DeltaTime;
    
    // Check if client has timed out
    if (TimeSinceLastClientActivity > ClientTimeoutSeconds)
    {
        MCP_LOG_WARNING("Client timed out after %.1f seconds of inactivity, disconnecting", TimeSinceLastClientActivity);
        CleanupClientConnection();
    }
}

void FMCPTCPServer::CleanupClientConnection()
{
    if (!ClientSocket) return;
    
    MCP_LOG_INFO("Cleaning up client connection");
    
    try
    {
        // Get the socket description before closing
        FString SocketDesc = GetSafeSocketDescription(ClientSocket);
        MCP_LOG_VERBOSE("Closing client socket with description: %s", *SocketDesc);
        
        // First close the socket
        bool bCloseSuccess = ClientSocket->Close();
        if (!bCloseSuccess)
        {
            MCP_LOG_ERROR("Failed to close client socket");
        }
        
        // Then destroy it
        ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        if (SocketSubsystem)
        {
            SocketSubsystem->DestroySocket(ClientSocket);
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
    
    // Reset the client socket pointer and timeout
    ClientSocket = nullptr;
    TimeSinceLastClientActivity = 0.0f;
    MCP_LOG_INFO("MCP Client disconnected");
}

void FMCPTCPServer::ProcessCommand(const FString& CommandJson)
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
                CommandHandlers[Type](*ParamsPtr);
            }
            else
            {
                // Empty params
                CommandHandlers[Type](MakeShared<FJsonObject>());
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

void FMCPTCPServer::HandleGetSceneInfo(const TSharedPtr<FJsonObject>& Params)
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

void FMCPTCPServer::HandleCreateObject(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    FString Type;
    Params->TryGetStringField("type", Type);
    
    FVector Location(0, 0, 0);
    const TArray<TSharedPtr<FJsonValue>>* LocationArray;
    if (Params->TryGetArrayField("location", LocationArray) && LocationArray->Num() >= 3)
    {
        Location.X = (*LocationArray)[0]->AsNumber();
        Location.Y = (*LocationArray)[1]->AsNumber();
        Location.Z = (*LocationArray)[2]->AsNumber();
    }

    AActor* NewActor = nullptr;
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP Create Object")));
    
    if (Type == "CUBE")
    {
        NewActor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
        AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(NewActor);
        UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
        MeshComp->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        MeshActor->SetActorLocation(Location);
        MeshActor->SetActorLabel(FString::Printf(TEXT("MCP_Cube_%d"), FMath::RandRange(1000, 9999)));
    }
    else if (Type == "SPHERE")
    {
        NewActor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
        AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(NewActor);
        UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
        MeshComp->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        MeshActor->SetActorLocation(Location);
        MeshActor->SetActorLabel(FString::Printf(TEXT("MCP_Sphere_%d"), FMath::RandRange(1000, 9999)));
    }
    else
    {
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "error");
        Response->SetStringField("message", FString::Printf(TEXT("Unknown object type: %s"), *Type));
        SendResponse(ClientSocket, Response);
        return;
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    if (NewActor)
    {
        Result->SetStringField("name", NewActor->GetName());
        Result->SetStringField("label", NewActor->GetActorLabel());
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField("status", "success");
    Response->SetObjectField("result", Result);
    SendResponse(ClientSocket, Response);
}

void FMCPTCPServer::HandleModifyObject(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    FString Name;
    if (!Params->TryGetStringField("name", Name))
    {
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "error");
        Response->SetStringField("message", TEXT("No object name specified"));
        SendResponse(ClientSocket, Response);
        return;
    }
    
    AActor* TargetActor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == Name || It->GetActorLabel() == Name)
        {
            TargetActor = *It;
            break;
        }
    }
    
    if (!TargetActor)
    {
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "error");
        Response->SetStringField("message", FString::Printf(TEXT("Object not found: %s"), *Name));
        SendResponse(ClientSocket, Response);
        return;
    }
    
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP Modify Object")));
    TargetActor->Modify();
    
    // Handle location change
    const TArray<TSharedPtr<FJsonValue>>* LocationArray;
    if (Params->TryGetArrayField("location", LocationArray) && LocationArray->Num() >= 3)
    {
        FVector NewLocation;
        NewLocation.X = (*LocationArray)[0]->AsNumber();
        NewLocation.Y = (*LocationArray)[1]->AsNumber();
        NewLocation.Z = (*LocationArray)[2]->AsNumber();
        TargetActor->SetActorLocation(NewLocation);
    }
    
    // Handle rotation change
    const TArray<TSharedPtr<FJsonValue>>* RotationArray;
    if (Params->TryGetArrayField("rotation", RotationArray) && RotationArray->Num() >= 3)
    {
        FRotator NewRotation;
        NewRotation.Pitch = (*RotationArray)[0]->AsNumber();
        NewRotation.Yaw = (*RotationArray)[1]->AsNumber();
        NewRotation.Roll = (*RotationArray)[2]->AsNumber();
        TargetActor->SetActorRotation(NewRotation);
    }
    
    // Handle scale change
    const TArray<TSharedPtr<FJsonValue>>* ScaleArray;
    if (Params->TryGetArrayField("scale", ScaleArray) && ScaleArray->Num() >= 3)
    {
        FVector NewScale;
        NewScale.X = (*ScaleArray)[0]->AsNumber();
        NewScale.Y = (*ScaleArray)[1]->AsNumber();
        NewScale.Z = (*ScaleArray)[2]->AsNumber();
        TargetActor->SetActorScale3D(NewScale);
    }
    
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField("name", TargetActor->GetName());
    
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField("status", "success");
    Response->SetObjectField("result", Result);
    SendResponse(ClientSocket, Response);
}

void FMCPTCPServer::HandleDeleteObject(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    FString Name;
    if (!Params->TryGetStringField("name", Name))
    {
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "error");
        Response->SetStringField("message", TEXT("No object name specified"));
        SendResponse(ClientSocket, Response);
        return;
    }
    
    AActor* TargetActor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == Name || It->GetActorLabel() == Name)
        {
            TargetActor = *It;
            break;
        }
    }
    
    if (!TargetActor)
    {
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "error");
        Response->SetStringField("message", FString::Printf(TEXT("Object not found: %s"), *Name));
        SendResponse(ClientSocket, Response);
        return;
    }
    
    FScopedTransaction Transaction(FText::FromString(TEXT("MCP Delete Object")));
    World->EditorDestroyActor(TargetActor, true);
    
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField("name", Name);
    
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField("status", "success");
    Response->SetObjectField("result", Result);
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
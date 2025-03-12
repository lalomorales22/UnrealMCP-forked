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

FMCPTCPServer::FMCPTCPServer(int32 InPort) : Port(InPort), Listener(nullptr), ClientSocket(nullptr), bRunning(false)
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
    if (bRunning) return true;
    
    Listener = new FTcpListener(FIPv4Endpoint(FIPv4Address::Any, Port));
    if (!Listener || !Listener->IsActive())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start MCP server on port %d"), Port);
        Stop();
        return false;
    }

    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FMCPTCPServer::Tick), 0.1f);
    bRunning = true;
    UE_LOG(LogTemp, Log, TEXT("MCP Server started on port %d"), Port);
    return true;
}

void FMCPTCPServer::Stop()
{
    if (ClientSocket)
    {
        ClientSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
        ClientSocket = nullptr;
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
    UE_LOG(LogTemp, Log, TEXT("MCP Server stopped"));
}

bool FMCPTCPServer::Tick(float DeltaTime)
{
    if (!bRunning) return false;
    ProcessPendingConnections();
    ProcessClientData();
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
    // Only accept one connection at a time
    if (ClientSocket != nullptr)
    {
        return false;
    }
    
    ClientSocket = InSocket;
    ClientSocket->SetNonBlocking(true);
    UE_LOG(LogTemp, Log, TEXT("MCP Client connected from %s"), *Endpoint.ToString());
    return true;
}

void FMCPTCPServer::ProcessClientData()
{
    if (!ClientSocket) return;
    
    uint32 PendingDataSize = 0;
    if (ClientSocket->HasPendingData(PendingDataSize))
    {
        int32 BytesRead = 0;
        if (ClientSocket->Recv(ReceiveBuffer.GetData(), ReceiveBuffer.Num(), BytesRead))
        {
            if (BytesRead > 0)
            {
                FString ReceivedData = FString(UTF8_TO_TCHAR(ReceiveBuffer.GetData()));
                ProcessCommand(ReceivedData);
            }
        }
        else
        {
            // Connection lost
            ClientSocket->Close();
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
            ClientSocket = nullptr;
            UE_LOG(LogTemp, Log, TEXT("MCP Client disconnected"));
        }
    }
}

void FMCPTCPServer::ProcessCommand(const FString& CommandJson)
{
    TSharedPtr<FJsonObject> Command;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CommandJson);
    if (FJsonSerializer::Deserialize(Reader, Command) && Command.IsValid())
    {
        FString Type;
        if (Command->TryGetStringField(FStringView(TEXT("type")), Type) && CommandHandlers.Contains(Type))
        {
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
            TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
            Response->SetStringField("status", "error");
            Response->SetStringField("message", FString::Printf(TEXT("Unknown command: %s"), *Type));
            SendResponse(ClientSocket, Response);
        }
    }
    else
    {
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "error");
        Response->SetStringField("message", TEXT("Invalid JSON format"));
        SendResponse(ClientSocket, Response);
    }
}

void FMCPTCPServer::SendResponse(FSocket* Client, const TSharedPtr<FJsonObject>& Response)
{
    if (!Client) return;
    
    FString ResponseStr;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseStr);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
    
    FTCHARToUTF8 Converter(*ResponseStr);
    int32 BytesSent = 0;
    Client->Send((uint8*)Converter.Get(), Converter.Length(), BytesSent);
}

void FMCPTCPServer::HandleGetSceneInfo(const TSharedPtr<FJsonObject>& Params)
{
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
    SendResponse(ClientSocket, Response);
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
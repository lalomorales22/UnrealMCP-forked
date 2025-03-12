#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Json.h"
#include "Networking.h"
#include "Common/TcpListener.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

class UNREALMCP_API FMCPTCPServer
{
public:
    FMCPTCPServer(int32 InPort);
    ~FMCPTCPServer();

    bool Start();
    void Stop();
    bool IsRunning() const { return bRunning; }

private:
    bool Tick(float DeltaTime);
    void ProcessPendingConnections();
    void ProcessClientData();
    void ProcessCommand(const FString& CommandJson);
    void SendResponse(FSocket* Client, const TSharedPtr<FJsonObject>& Response);
    
    // Connection handler
    bool HandleConnectionAccepted(FSocket* InSocket, const FIPv4Endpoint& Endpoint);

    // Command handlers
    void HandleGetSceneInfo(const TSharedPtr<FJsonObject>& Params);
    void HandleCreateObject(const TSharedPtr<FJsonObject>& Params);
    void HandleModifyObject(const TSharedPtr<FJsonObject>& Params);
    void HandleDeleteObject(const TSharedPtr<FJsonObject>& Params);

    FTcpListener* Listener;
    FSocket* ClientSocket;
    TArray<uint8> ReceiveBuffer;
    int32 Port;
    bool bRunning;
    FTSTicker::FDelegateHandle TickerHandle;

    TMap<FString, TFunction<void(const TSharedPtr<FJsonObject>&)>> CommandHandlers;
}; 
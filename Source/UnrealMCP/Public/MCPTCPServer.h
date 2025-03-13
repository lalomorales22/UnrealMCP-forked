#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Json.h"
#include "Networking.h"
#include "Common/TcpListener.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

// Structure to track client connection information
struct FMCPClientConnection
{
    FSocket* Socket;
    FIPv4Endpoint Endpoint;
    float TimeSinceLastActivity;
    TArray<uint8> ReceiveBuffer;

    FMCPClientConnection(FSocket* InSocket, const FIPv4Endpoint& InEndpoint)
        : Socket(InSocket)
        , Endpoint(InEndpoint)
        , TimeSinceLastActivity(0.0f)
    {
        ReceiveBuffer.SetNumUninitialized(8192); // 8KB buffer
    }
};

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
    void ProcessCommand(const FString& CommandJson, FSocket* ClientSocket);
    void SendResponse(FSocket* Client, const TSharedPtr<FJsonObject>& Response);
    void CheckClientTimeouts(float DeltaTime);
    void CleanupClientConnection(FMCPClientConnection& ClientConnection);
    void CleanupClientConnection(FSocket* ClientSocket);
    void CleanupAllClientConnections();
    FString GetSafeSocketDescription(FSocket* Socket);
    
    // Connection handler
    bool HandleConnectionAccepted(FSocket* InSocket, const FIPv4Endpoint& Endpoint);

    // Command handlers
    void HandleGetSceneInfo(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket);
    void HandleCreateObject(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket);
    void HandleModifyObject(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket);
    void HandleDeleteObject(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket);

    FTcpListener* Listener;
    TArray<FMCPClientConnection> ClientConnections;
    int32 Port;
    bool bRunning;
    FTSTicker::FDelegateHandle TickerHandle;
    float ClientTimeoutSeconds;

    TMap<FString, TFunction<void(const TSharedPtr<FJsonObject>&, FSocket*)>> CommandHandlers;
}; 
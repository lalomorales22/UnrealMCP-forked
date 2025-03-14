#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Json.h"
#include "Networking.h"
#include "Common/TcpListener.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "MCPConstants.h"

/**
 * Configuration struct for the TCP server
 * Allows for easy customization of server parameters
 */
struct FMCPTCPServerConfig
{
    /** Port to listen on */
    int32 Port = MCPConstants::DEFAULT_PORT;
    
    /** Client timeout in seconds */
    float ClientTimeoutSeconds = MCPConstants::DEFAULT_CLIENT_TIMEOUT_SECONDS;
    
    /** Size of the receive buffer in bytes */
    int32 ReceiveBufferSize = MCPConstants::DEFAULT_RECEIVE_BUFFER_SIZE;
    
    /** Tick interval in seconds */
    float TickIntervalSeconds = MCPConstants::DEFAULT_TICK_INTERVAL_SECONDS;
    
    /** Whether to log verbose messages */
    bool bEnableVerboseLogging = MCPConstants::DEFAULT_VERBOSE_LOGGING;
};

/**
 * Structure to track client connection information
 */
struct FMCPClientConnection
{
    /** Socket for this client */
    FSocket* Socket;
    
    /** Endpoint information */
    FIPv4Endpoint Endpoint;
    
    /** Time since last activity for timeout tracking */
    float TimeSinceLastActivity;
    
    /** Buffer for receiving data */
    TArray<uint8> ReceiveBuffer;

    /**
     * Constructor
     * @param InSocket - The client socket
     * @param InEndpoint - The client endpoint
     * @param BufferSize - Size of the receive buffer
     */
    FMCPClientConnection(FSocket* InSocket, const FIPv4Endpoint& InEndpoint, int32 BufferSize = MCPConstants::DEFAULT_RECEIVE_BUFFER_SIZE)
        : Socket(InSocket)
        , Endpoint(InEndpoint)
        , TimeSinceLastActivity(0.0f)
    {
        ReceiveBuffer.SetNumUninitialized(BufferSize);
    }
};

/**
 * Interface for command handlers
 * Allows for easy addition of new commands without modifying the server
 */
class IMCPCommandHandler
{
public:
    virtual ~IMCPCommandHandler() {}
    
    /**
     * Get the command name this handler responds to
     * @return The command name
     */
    virtual FString GetCommandName() const = 0;
    
    /**
     * Handle the command
     * @param Params - The command parameters
     * @param ClientSocket - The client socket
     * @return JSON response object
     */
    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) = 0;
};

/**
 * MCP TCP Server
 * Manages connections and command routing
 */
class UNREALMCP_API FMCPTCPServer
{
public:
    /**
     * Constructor
     * @param InConfig - Configuration for the server
     */
    FMCPTCPServer(const FMCPTCPServerConfig& InConfig);
    
    /**
     * Destructor
     */
    virtual ~FMCPTCPServer();
    
    /**
     * Start the server
     * @return True if started successfully
     */
    bool Start();
    
    /**
     * Stop the server
     */
    void Stop();
    
    /**
     * Check if the server is running
     * @return True if running
     */
    bool IsRunning() const { return bRunning; }
    
    /**
     * Register a command handler
     * @param Handler - The handler to register
     */
    void RegisterCommandHandler(TSharedPtr<IMCPCommandHandler> Handler);
    
    /**
     * Unregister a command handler
     * @param CommandName - The command name to unregister
     */
    void UnregisterCommandHandler(const FString& CommandName);

    /**
     * Register an external command handler
     * This is a public API that allows external code to extend the MCP plugin with custom functionality
     * @param Handler - The handler to register
     * @return True if registration was successful
     */
    bool RegisterExternalCommandHandler(TSharedPtr<IMCPCommandHandler> Handler);

    /**
     * Unregister an external command handler
     * @param CommandName - The command name to unregister
     * @return True if unregistration was successful
     */
    bool UnregisterExternalCommandHandler(const FString& CommandName);

    /**
     * Send a response to a client
     * @param Client - The client socket
     * @param Response - The response to send
     */
    void SendResponse(FSocket* Client, const TSharedPtr<FJsonObject>& Response);

    /**
     * Get the command handlers map (for testing purposes)
     * @return The map of command handlers
     */
    const TMap<FString, TSharedPtr<IMCPCommandHandler>>& GetCommandHandlers() const { return CommandHandlers; }

protected:
    /**
     * Tick function called by the ticker
     * @param DeltaTime - Time since last tick
     * @return True to continue ticking
     */
    bool Tick(float DeltaTime);
    
    /**
     * Process pending connections
     */
    virtual void ProcessPendingConnections();
    
    /**
     * Process client data
     */
    virtual void ProcessClientData();
    
    /**
     * Process a command
     * @param CommandJson - The command JSON
     * @param ClientSocket - The client socket
     */
    virtual void ProcessCommand(const FString& CommandJson, FSocket* ClientSocket);
    
    /**
     * Check for client timeouts
     * @param DeltaTime - Time since last tick
     */
    virtual void CheckClientTimeouts(float DeltaTime);
    
    /**
     * Clean up a client connection
     * @param ClientConnection - The client connection to clean up
     */
    virtual void CleanupClientConnection(FMCPClientConnection& ClientConnection);
    
    /**
     * Clean up a client connection by socket
     * @param ClientSocket - The client socket to clean up
     */
    virtual void CleanupClientConnection(FSocket* ClientSocket);
    
    /**
     * Clean up all client connections
     */
    virtual void CleanupAllClientConnections();
    
    /**
     * Get a safe description of a socket
     * @param Socket - The socket
     * @return A safe description string
     */
    FString GetSafeSocketDescription(FSocket* Socket);
    
    /**
     * Connection handler
     * @param InSocket - The new client socket
     * @param Endpoint - The client endpoint
     * @return True if connection accepted
     */
    virtual bool HandleConnectionAccepted(FSocket* InSocket, const FIPv4Endpoint& Endpoint);

    /** Server configuration */
    FMCPTCPServerConfig Config;
    
    /** TCP listener */
    FTcpListener* Listener;
    
    /** Client connections */
    TArray<FMCPClientConnection> ClientConnections;
    
    /** Running flag */
    bool bRunning;
    
    /** Ticker handle */
    FTSTicker::FDelegateHandle TickerHandle;
    
    /** Command handlers map */
    TMap<FString, TSharedPtr<IMCPCommandHandler>> CommandHandlers;

private:
    // Disable copy and assignment
    FMCPTCPServer(const FMCPTCPServer&) = delete;
    FMCPTCPServer& operator=(const FMCPTCPServer&) = delete;
}; 
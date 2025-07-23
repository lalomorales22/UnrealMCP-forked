#pragma once

#include "CoreMinimal.h"
#include "MCPTCPServer.h"
#include "Delegates/Delegate.h"
#include "Json.h"

/**
 * Delegate for handling MCP command execution
 * Used by the extension system to allow easy registration of custom command handlers
 */
DECLARE_DELEGATE_RetVal_TwoParams(
    TSharedPtr<FJsonObject>,                 // Return type: JSON response
    FMCPCommandExecuteDelegate,              // Delegate name
    const TSharedPtr<FJsonObject>&,          // Parameter 1: Command parameters
    FSocket*                                 // Parameter 2: Client socket
);

/**
 * Helper class for creating external command handlers
 * Makes it easy for external code to register custom commands with the MCP server
 */
class UNREALARCHITECT_API FMCPExtensionHandler : public IMCPCommandHandler
{
public:
    /**
     * Constructor
     * @param InCommandName - The command name this handler responds to
     * @param InExecuteDelegate - The delegate to execute when this command is received
     */
    FMCPExtensionHandler(const FString& InCommandName, const FMCPCommandExecuteDelegate& InExecuteDelegate)
        : CommandName(InCommandName)
        , ExecuteDelegate(InExecuteDelegate)
    {
    }

    

    /**
     * Get the command name this handler responds to
     * @return The command name
     */
    virtual FString GetCommandName() const override
    {
        return CommandName;
    }

    /**
     * Handle the command by executing the delegate
     * @param Params - The command parameters
     * @param ClientSocket - The client socket
     * @return JSON response object
     */
    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
    {
        // If the delegate is bound, execute it
        if (ExecuteDelegate.IsBound())
        {
            return ExecuteDelegate.Execute(Params, ClientSocket);
        }
        
        // If the delegate is not bound, return an error
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "error");
        Response->SetStringField("message", FString::Printf(TEXT("Command handler for '%s' has no bound execution delegate"), *CommandName));
        return Response;
    }

private:
    /** The command name this handler responds to */
    FString CommandName;
    
    /** The delegate to execute when this command is received */
    FMCPCommandExecuteDelegate ExecuteDelegate;
};

/**
 * Helper utility for working with the MCP extension system
 */
class UNREALARCHITECT_API FMCPExtensionSystem
{
public:
    /**
     * Register a command handler with the server
     * @param Server - The MCP server
     * @param CommandName - The name of the command to register
     * @param ExecuteDelegate - The delegate to execute when the command is received
     * @return True if registration was successful
     */
    static bool RegisterCommand(FMCPTCPServer* Server, const FString& CommandName, const FMCPCommandExecuteDelegate& ExecuteDelegate)
    {
        if (!Server)
        {
            return false;
        }
        
        // Create a handler with the delegate
        TSharedPtr<FMCPExtensionHandler> Handler = MakeShared<FMCPExtensionHandler>(CommandName, ExecuteDelegate);
        
        // Register the handler with the server
        return Server->RegisterExternalCommandHandler(Handler);
    }

    /**
     * Unregister a command handler with the server
     * @param Server - The MCP server
     * @param CommandName - The name of the command to unregister
     * @return True if unregistration was successful
     */
    static bool UnregisterCommand(FMCPTCPServer* Server, const FString& CommandName)
    {
        if (!Server)
        {
            return false;
        }
        
        // Unregister the handler with the server
        return Server->UnregisterExternalCommandHandler(CommandName);
    }
}; 
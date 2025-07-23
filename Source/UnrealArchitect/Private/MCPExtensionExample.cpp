#include "MCPExtensionHandler.h"
#include "MCPFileLogger.h"

/**
 * Example showing how to use the MCP extension system
 * 
 * This code demonstrates how external modules can extend the MCP server
 * with custom commands without modifying the MCP plugin code.
 */
class FMCPExtensionExample
{
public:
    static void RegisterCustomCommands(FMCPTCPServer* Server)
    {
        if (!Server || !Server->IsRunning())
        {
            return;
        }

        // Register a custom "hello_world" command
        FMCPExtensionSystem::RegisterCommand(
            Server,
            "hello_world",
            FMCPCommandExecuteDelegate::CreateStatic(&FMCPExtensionExample::HandleHelloWorldCommand)
        );
        
        // Register a custom "echo" command
        FMCPExtensionSystem::RegisterCommand(
            Server,
            "echo",
            FMCPCommandExecuteDelegate::CreateStatic(&FMCPExtensionExample::HandleEchoCommand)
        );
    }

    static void UnregisterCustomCommands(FMCPTCPServer* Server)
    {
        if (!Server)
        {
            return;
        }

        // Unregister the custom commands
        FMCPExtensionSystem::UnregisterCommand(Server, "hello_world");
        FMCPExtensionSystem::UnregisterCommand(Server, "echo");
    }

private:
    /**
     * Handle the "hello_world" command
     * @param Params - The command parameters
     * @param ClientSocket - The client socket
     * @return JSON response
     */
    static TSharedPtr<FJsonObject> HandleHelloWorldCommand(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
    {
        // Log that we received this command
        UE_LOG(LogMCP, Display, TEXT("Received hello_world command"));
        
        // Get the name parameter if provided
        FString Name = "World";
        Params->TryGetStringField(FStringView(TEXT("name")), Name);
        
        // Create the response
        TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetStringField("message", FString::Printf(TEXT("Hello, %s!"), *Name));
        
        // Create the success response with the result
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "success");
        Response->SetObjectField("result", Result);
        
        return Response;
    }
    
    /**
     * Handle the "echo" command
     * @param Params - The command parameters
     * @param ClientSocket - The client socket
     * @return JSON response
     */
    static TSharedPtr<FJsonObject> HandleEchoCommand(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
    {
        // Log that we received this command
        UE_LOG(LogMCP, Display, TEXT("Received echo command"));
        
        // Create the response
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "success");
        
        // Echo back all parameters as the result
        Response->SetObjectField("result", Params);
        
        return Response;
    }
};

// The following code shows how you might register these handlers in your own module
// Uncomment this code and modify as needed for your project

/*
void YourGameModule::StartupModule()
{
    // ... your existing code ...
    
    // Get a reference to the MCP server
    FUnrealArchitectModule& MCPModule = FModuleManager::LoadModuleChecked<FUnrealArchitectModule>("UnrealArchitect");
    FMCPTCPServer* MCPServer = MCPModule.GetServer();
    
    if (MCPServer && MCPServer->IsRunning())
    {
        // Register custom commands
        FMCPExtensionExample::RegisterCustomCommands(MCPServer);
    }
    else
    {
        // The MCP server isn't running yet
        // You might want to set up a delegate to register when it starts
        // or expose a function in the MCP module that lets you register commands
        // that will be applied when the server starts
    }
}

void YourGameModule::ShutdownModule()
{
    // Get a reference to the MCP server
    FUnrealArchitectModule& MCPModule = FModuleManager::GetModulePtr<FUnrealArchitectModule>("UnrealArchitect");
    if (MCPModule)
    {
        FMCPTCPServer* MCPServer = MCPModule.GetServer();
        if (MCPServer)
        {
            // Unregister custom commands
            FMCPExtensionExample::UnregisterCustomCommands(MCPServer);
        }
    }
    
    // ... your existing code ...
}
*/ 
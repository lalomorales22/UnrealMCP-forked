#pragma once

#include "CoreMinimal.h"
#include "MCPTCPServer.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "ActorEditorUtils.h"
#include "EngineUtils.h"

/**
 * Base class for MCP command handlers
 */
class FMCPCommandHandlerBase : public IMCPCommandHandler
{
public:
    /**
     * Constructor
     * @param InCommandName - The command name this handler responds to
     */
    explicit FMCPCommandHandlerBase(const FString& InCommandName)
        : CommandName(InCommandName)
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

protected:
    /**
     * Create an error response
     * @param Message - The error message
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateErrorResponse(const FString& Message)
    {
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "error");
        Response->SetStringField("message", Message);
        return Response;
    }

    /**
     * Create a success response
     * @param Result - Optional result object
     * @return JSON response object
     */
    TSharedPtr<FJsonObject> CreateSuccessResponse(TSharedPtr<FJsonObject> Result = nullptr)
    {
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "success");
        if (Result.IsValid())
        {
            Response->SetObjectField("result", Result);
        }
        return Response;
    }

    /** The command name this handler responds to */
    FString CommandName;
};

/**
 * Handler for the get_scene_info command
 */
class FMCPGetSceneInfoHandler : public FMCPCommandHandlerBase
{
public:
    FMCPGetSceneInfoHandler()
        : FMCPCommandHandlerBase("get_scene_info")
    {
    }

    /**
     * Handle the get_scene_info command
     * @param Params - The command parameters
     * @param ClientSocket - The client socket
     * @return JSON response object
     */
    virtual TSharedPtr<FJsonObject> HandleCommand(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;
};

/**
 * Handler for the create_object command
 */
class FMCPCreateObjectHandler : public FMCPCommandHandlerBase
{
public:
    FMCPCreateObjectHandler()
        : FMCPCommandHandlerBase("create_object")
    {
    }

    /**
     * Handle the create_object command
     * @param Params - The command parameters
     * @param ClientSocket - The client socket
     * @return JSON response object
     */
    virtual TSharedPtr<FJsonObject> HandleCommand(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;

protected:
    /**
     * Create a static mesh actor
     * @param World - The world to create the actor in
     * @param Location - The location to create the actor at
     * @param MeshPath - Optional path to the mesh to use
     * @return The created actor and a success flag
     */
    TPair<AStaticMeshActor*, bool> CreateStaticMeshActor(UWorld* World, const FVector& Location, const FString& MeshPath = "");

    /**
     * Create a cube actor
     * @param World - The world to create the actor in
     * @param Location - The location to create the actor at
     * @return The created actor and a success flag
     */
    TPair<AStaticMeshActor*, bool> CreateCubeActor(UWorld* World, const FVector& Location);
};

/**
 * Handler for the modify_object command
 */
class FMCPModifyObjectHandler : public FMCPCommandHandlerBase
{
public:
    FMCPModifyObjectHandler()
        : FMCPCommandHandlerBase("modify_object")
    {
    }

    /**
     * Handle the modify_object command
     * @param Params - The command parameters
     * @param ClientSocket - The client socket
     * @return JSON response object
     */
    virtual TSharedPtr<FJsonObject> HandleCommand(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;
};

/**
 * Handler for the delete_object command
 */
class FMCPDeleteObjectHandler : public FMCPCommandHandlerBase
{
public:
    FMCPDeleteObjectHandler()
        : FMCPCommandHandlerBase("delete_object")
    {
    }

    /**
     * Handle the delete_object command
     * @param Params - The command parameters
     * @param ClientSocket - The client socket
     * @return JSON response object
     */
    virtual TSharedPtr<FJsonObject> HandleCommand(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;
}; 
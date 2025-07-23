#pragma once

#include "CoreMinimal.h"
#include "MCPCommandHandlers.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "EdGraphSchema_K2.h"
#include "AssetRegistry/AssetRegistryModule.h"

/**
 * Common utilities for blueprint operations
 */
class FMCPBlueprintUtils
{
public:
    /**
     * Create a new blueprint asset
     * @param PackagePath - Path where the blueprint should be created
     * @param BlueprintName - Name of the blueprint
     * @param ParentClass - Parent class for the blueprint
     * @return The created blueprint and success flag
     */
    static TPair<UBlueprint*, bool> CreateBlueprintAsset(
        const FString& PackagePath,
        const FString& BlueprintName,
        UClass* ParentClass);

    /**
     * Add event node to blueprint
     * @param Blueprint - Target blueprint
     * @param EventName - Name of the event to create
     * @param ParentClass - Parent class containing the event
     * @return The created event node and success flag
     */
    static TPair<UK2Node_Event*, bool> AddEventNode(
        UBlueprint* Blueprint,
        const FString& EventName,
        UClass* ParentClass);

    /**
     * Add print string node to blueprint
     * @param Graph - Target graph
     * @param Message - Message to print
     * @return The created print node and success flag
     */
    static TPair<UK2Node_CallFunction*, bool> AddPrintStringNode(
        UEdGraph* Graph,
        const FString& Message);
};

/**
 * Handler for creating blueprints
 */
class FMCPCreateBlueprintHandler : public FMCPCommandHandlerBase
{
public:
    FMCPCreateBlueprintHandler() : FMCPCommandHandlerBase(TEXT("create_blueprint")) {}
    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;

private:
    TPair<UBlueprint*, bool> CreateBlueprint(const FString& PackagePath, const FString& BlueprintName, const TSharedPtr<FJsonObject>& Properties);
};

/**
 * Handler for modifying blueprints
 */
class FMCPModifyBlueprintHandler : public FMCPCommandHandlerBase
{
public:
    FMCPModifyBlueprintHandler() : FMCPCommandHandlerBase(TEXT("modify_blueprint")) {}
    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;

private:
    bool ModifyBlueprint(UBlueprint* Blueprint, const TSharedPtr<FJsonObject>& Properties);
};

/**
 * Handler for getting blueprint info
 */
class FMCPGetBlueprintInfoHandler : public FMCPCommandHandlerBase
{
public:
    FMCPGetBlueprintInfoHandler() : FMCPCommandHandlerBase(TEXT("get_blueprint_info")) {}
    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;

private:
    TSharedPtr<FJsonObject> GetBlueprintInfo(UBlueprint* Blueprint);
};

/**
 * Handler for creating blueprint events
 */
class FMCPCreateBlueprintEventHandler : public FMCPCommandHandlerBase
{
public:
    FMCPCreateBlueprintEventHandler() : FMCPCommandHandlerBase(TEXT("create_blueprint_event")) {}
    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;

private:
    TPair<bool, TSharedPtr<FJsonObject>> CreateBlueprintEvent(
        UWorld* World,
        const FString& EventName,
        const FString& BlueprintPath,
        const TSharedPtr<FJsonObject>& EventParameters);
}; 
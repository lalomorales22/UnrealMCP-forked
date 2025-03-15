#pragma once

#include "CoreMinimal.h"
#include "MCPCommandHandlers.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"

class FMCPCreateMaterialHandler : public FMCPCommandHandlerBase
{
public:
    FMCPCreateMaterialHandler() : FMCPCommandHandlerBase(TEXT("create_material")) {}
    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;

private:
    TPair<UMaterial*, bool> CreateMaterial(const FString& PackagePath, const FString& MaterialName, const TSharedPtr<FJsonObject>& Properties);
    bool ModifyMaterialProperties(UMaterial* Material, const TSharedPtr<FJsonObject>& Properties);
};

class FMCPModifyMaterialHandler : public FMCPCommandHandlerBase
{
public:
    FMCPModifyMaterialHandler() : FMCPCommandHandlerBase(TEXT("modify_material")) {}
    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;

private:
    bool ModifyMaterialProperties(UMaterial* Material, const TSharedPtr<FJsonObject>& Properties);
};

class FMCPGetMaterialInfoHandler : public FMCPCommandHandlerBase
{
public:
    FMCPGetMaterialInfoHandler() : FMCPCommandHandlerBase(TEXT("get_material_info")) {}
    virtual TSharedPtr<FJsonObject> Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket) override;

private:
    TSharedPtr<FJsonObject> GetMaterialInfo(UMaterial* Material);
}; 
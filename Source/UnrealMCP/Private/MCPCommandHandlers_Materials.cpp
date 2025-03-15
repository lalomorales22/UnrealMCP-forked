#include "MCPCommandHandlers_Materials.h"
#include "MCPCommandHandlers.h"
#include "Editor.h"
#include "MCPFileLogger.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "MCPConstants.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UObject/SavePackage.h"


//
// FMCPCreateMaterialHandler
//
TSharedPtr<FJsonObject> FMCPCreateMaterialHandler::Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    MCP_LOG_INFO("Handling create_material command");

    FString PackagePath;
    if (!Params->TryGetStringField(FStringView(TEXT("package_path")), PackagePath))
    {
        MCP_LOG_WARNING("Missing 'package_path' field in create_material command");
        return CreateErrorResponse("Missing 'package_path' field");
    }

    FString MaterialName;
    if (!Params->TryGetStringField(FStringView(TEXT("name")), MaterialName))
    {
        MCP_LOG_WARNING("Missing 'name' field in create_material command");
        return CreateErrorResponse("Missing 'name' field");
    }

    // Get optional properties
    const TSharedPtr<FJsonObject>* Properties = nullptr;
    Params->TryGetObjectField(FStringView(TEXT("properties")), Properties);

    // Create the material
    TPair<UMaterial*, bool> Result = CreateMaterial(PackagePath, MaterialName, Properties ? *Properties : nullptr);

    if (Result.Value)
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField("name", Result.Key->GetName());
        ResultObj->SetStringField("path", Result.Key->GetPathName());
        return CreateSuccessResponse(ResultObj);
    }
    else
    {
        return CreateErrorResponse("Failed to create material");
    }
}

TPair<UMaterial*, bool> FMCPCreateMaterialHandler::CreateMaterial(const FString& PackagePath, const FString& MaterialName, const TSharedPtr<FJsonObject>& Properties)
{
    // Create the package path
    FString FullPath = FPaths::Combine(PackagePath, MaterialName);
    UPackage* Package = CreatePackage(*FullPath);
    if (!Package)
    {
        MCP_LOG_ERROR("Failed to create package at path: %s", *FullPath);
        return TPair<UMaterial*, bool>(nullptr, false);
    }

    // Create the material
    UMaterial* NewMaterial = NewObject<UMaterial>(Package, *MaterialName, RF_Public | RF_Standalone);
    if (!NewMaterial)
    {
        MCP_LOG_ERROR("Failed to create material: %s", *MaterialName);
        return TPair<UMaterial*, bool>(nullptr, false);
    }

    // Set default properties
    NewMaterial->SetShadingModel(MSM_DefaultLit);
    NewMaterial->BlendMode = BLEND_Opaque;
    NewMaterial->TwoSided = false;
    NewMaterial->DitheredLODTransition = false;
    NewMaterial->bCastDynamicShadowAsMasked = false;

    // Apply any custom properties if provided
    if (Properties)
    {
        ModifyMaterialProperties(NewMaterial, Properties);
    }

    // Save the package
    Package->SetDirtyFlag(true);
    
    // Construct the full file path for saving
    FString SavePath = FPaths::Combine(FPaths::ProjectContentDir(), PackagePath, MaterialName + TEXT(".uasset"));
    
    // Create save package args
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    SaveArgs.bForceByteSwapping = false;
    SaveArgs.bWarnOfLongFilename = true;
    
    // Save the package
    if (!UPackage::SavePackage(Package, NewMaterial, *SavePath, SaveArgs))
    {
        MCP_LOG_ERROR("Failed to save material package at path: %s", *SavePath);
        return TPair<UMaterial*, bool>(nullptr, false);
    }
    
    // Trigger material compilation
    NewMaterial->PostEditChange();

    MCP_LOG_INFO("Created material: %s at path: %s", *MaterialName, *FullPath);
    return TPair<UMaterial*, bool>(NewMaterial, true);
}

bool FMCPCreateMaterialHandler::ModifyMaterialProperties(UMaterial* Material, const TSharedPtr<FJsonObject>& Properties)
{
    if (!Material || !Properties)
    {
        return false;
    }

    bool bSuccess = true;

    // Shading Model
    FString ShadingModel;
    if (Properties->TryGetStringField(FStringView(TEXT("shading_model")), ShadingModel))
    {
        if (ShadingModel == "DefaultLit")
            Material->SetShadingModel(MSM_DefaultLit);
        else if (ShadingModel == "Unlit")
            Material->SetShadingModel(MSM_Unlit);
        else if (ShadingModel == "Subsurface")
            Material->SetShadingModel(MSM_Subsurface);
        else if (ShadingModel == "PreintegratedSkin")
            Material->SetShadingModel(MSM_PreintegratedSkin);
        else if (ShadingModel == "ClearCoat")
            Material->SetShadingModel(MSM_ClearCoat);
        else if (ShadingModel == "SubsurfaceProfile")
            Material->SetShadingModel(MSM_SubsurfaceProfile);
        else if (ShadingModel == "TwoSidedFoliage")
            Material->SetShadingModel(MSM_TwoSidedFoliage);
        else if (ShadingModel == "Hair")
            Material->SetShadingModel(MSM_Hair);
        else if (ShadingModel == "Cloth")
            Material->SetShadingModel(MSM_Cloth);
        else if (ShadingModel == "Eye")
            Material->SetShadingModel(MSM_Eye);
        else
            bSuccess = false;
    }

    // Blend Mode
    FString BlendMode;
    if (Properties->TryGetStringField(FStringView(TEXT("blend_mode")), BlendMode))
    {
        if (BlendMode == "Opaque")
            Material->BlendMode = BLEND_Opaque;
        else if (BlendMode == "Masked")
            Material->BlendMode = BLEND_Masked;
        else if (BlendMode == "Translucent")
            Material->BlendMode = BLEND_Translucent;
        else if (BlendMode == "Additive")
            Material->BlendMode = BLEND_Additive;
        else if (BlendMode == "Modulate")
            Material->BlendMode = BLEND_Modulate;
        else if (BlendMode == "AlphaComposite")
            Material->BlendMode = BLEND_AlphaComposite;
        else if (BlendMode == "AlphaHoldout")
            Material->BlendMode = BLEND_AlphaHoldout;
        else
            bSuccess = false;
    }

    // Two Sided
    bool bTwoSided;
    if (Properties->TryGetBoolField(FStringView(TEXT("two_sided")), bTwoSided))
    {
        Material->TwoSided = bTwoSided;
    }

    // Dithered LOD Transition
    bool bDitheredLODTransition;
    if (Properties->TryGetBoolField(FStringView(TEXT("dithered_lod_transition")), bDitheredLODTransition))
    {
        Material->DitheredLODTransition = bDitheredLODTransition;
    }

    // Cast Contact Shadow
    bool bCastContactShadow;
    if (Properties->TryGetBoolField(FStringView(TEXT("cast_contact_shadow")), bCastContactShadow))
    {
        Material->bCastDynamicShadowAsMasked = bCastContactShadow;
    }

    // Base Color
    const TArray<TSharedPtr<FJsonValue>>* BaseColorArray = nullptr;
    if (Properties->TryGetArrayField(FStringView(TEXT("base_color")), BaseColorArray) && BaseColorArray && BaseColorArray->Num() == 4)
    {
        FLinearColor BaseColor(
            (*BaseColorArray)[0]->AsNumber(),
            (*BaseColorArray)[1]->AsNumber(),
            (*BaseColorArray)[2]->AsNumber(),
            (*BaseColorArray)[3]->AsNumber()
        );
        
        // Create a Vector4 constant expression for base color
        UMaterialExpressionVectorParameter* BaseColorParam = NewObject<UMaterialExpressionVectorParameter>(Material);
        BaseColorParam->ParameterName = TEXT("BaseColor");
        BaseColorParam->DefaultValue = BaseColor;
        Material->GetExpressionCollection().AddExpression(BaseColorParam);
        Material->GetEditorOnlyData()->BaseColor.Expression = BaseColorParam;
    }

    // Metallic
    double Metallic;
    if (Properties->TryGetNumberField(FStringView(TEXT("metallic")), Metallic))
    {
        // Create a scalar constant expression for metallic
        UMaterialExpressionScalarParameter* MetallicParam = NewObject<UMaterialExpressionScalarParameter>(Material);
        MetallicParam->ParameterName = TEXT("Metallic");
        MetallicParam->DefaultValue = FMath::Clamp(Metallic, 0.0, 1.0);
        Material->GetExpressionCollection().AddExpression(MetallicParam);
        Material->GetEditorOnlyData()->Metallic.Expression = MetallicParam;
    }

    // Roughness
    double Roughness;
    if (Properties->TryGetNumberField(FStringView(TEXT("roughness")), Roughness))
    {
        // Create a scalar constant expression for roughness
        UMaterialExpressionScalarParameter* RoughnessParam = NewObject<UMaterialExpressionScalarParameter>(Material);
        RoughnessParam->ParameterName = TEXT("Roughness");
        RoughnessParam->DefaultValue = FMath::Clamp(Roughness, 0.0, 1.0);
        Material->GetExpressionCollection().AddExpression(RoughnessParam);
        Material->GetEditorOnlyData()->Roughness.Expression = RoughnessParam;
    }

    return bSuccess;
}

//
// FMCPModifyMaterialHandler
//
TSharedPtr<FJsonObject> FMCPModifyMaterialHandler::Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    MCP_LOG_INFO("Handling modify_material command");

    FString MaterialPath;
    if (!Params->TryGetStringField(FStringView(TEXT("path")), MaterialPath))
    {
        MCP_LOG_WARNING("Missing 'path' field in modify_material command");
        return CreateErrorResponse("Missing 'path' field");
    }

    const TSharedPtr<FJsonObject>* Properties = nullptr;
    if (!Params->TryGetObjectField(FStringView(TEXT("properties")), Properties))
    {
        MCP_LOG_WARNING("Missing 'properties' field in modify_material command");
        return CreateErrorResponse("Missing 'properties' field");
    }

    // Load the material
    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Material)
    {
        MCP_LOG_ERROR("Failed to load material at path: %s", *MaterialPath);
        return CreateErrorResponse(FString::Printf(TEXT("Failed to load material at path: %s"), *MaterialPath));
    }

    // Modify the material properties
    bool bSuccess = ModifyMaterialProperties(Material, *Properties);

    if (bSuccess)
    {
        // Save the package
        Material->GetPackage()->SetDirtyFlag(true);
        
        // Create save package args
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;
        SaveArgs.bForceByteSwapping = false;
        SaveArgs.bWarnOfLongFilename = true;
        
        // Construct the full file path for saving
        FString SavePath = FPaths::Combine(FPaths::ProjectContentDir(), Material->GetPathName() + TEXT(".uasset"));
        
        // Save the package with the proper args
        if (!UPackage::SavePackage(Material->GetPackage(), Material, *SavePath, SaveArgs))
        {
            MCP_LOG_ERROR("Failed to save material package at path: %s", *SavePath);
            return CreateErrorResponse("Failed to save material package");
        }

        // Trigger material compilation
        Material->PostEditChange();

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField("name", Material->GetName());
        ResultObj->SetStringField("path", Material->GetPathName());
        return CreateSuccessResponse(ResultObj);
    }
    else
    {
        return CreateErrorResponse("Failed to modify material properties");
    }
}

bool FMCPModifyMaterialHandler::ModifyMaterialProperties(UMaterial* Material, const TSharedPtr<FJsonObject>& Properties)
{
    if (!Material || !Properties)
    {
        return false;
    }

    bool bSuccess = true;

    // Shading Model
    FString ShadingModel;
    if (Properties->TryGetStringField(FStringView(TEXT("shading_model")), ShadingModel))
    {
        if (ShadingModel == "DefaultLit")
            Material->SetShadingModel(MSM_DefaultLit);
        else if (ShadingModel == "Unlit")
            Material->SetShadingModel(MSM_Unlit);
        else if (ShadingModel == "Subsurface")
            Material->SetShadingModel(MSM_Subsurface);
        else if (ShadingModel == "PreintegratedSkin")
            Material->SetShadingModel(MSM_PreintegratedSkin);
        else if (ShadingModel == "ClearCoat")
            Material->SetShadingModel(MSM_ClearCoat);
        else if (ShadingModel == "SubsurfaceProfile")
            Material->SetShadingModel(MSM_SubsurfaceProfile);
        else if (ShadingModel == "TwoSidedFoliage")
            Material->SetShadingModel(MSM_TwoSidedFoliage);
        else if (ShadingModel == "Hair")
            Material->SetShadingModel(MSM_Hair);
        else if (ShadingModel == "Cloth")
            Material->SetShadingModel(MSM_Cloth);
        else if (ShadingModel == "Eye")
            Material->SetShadingModel(MSM_Eye);
        else
            bSuccess = false;
    }

    // Blend Mode
    FString BlendMode;
    if (Properties->TryGetStringField(FStringView(TEXT("blend_mode")), BlendMode))
    {
        if (BlendMode == "Opaque")
            Material->BlendMode = BLEND_Opaque;
        else if (BlendMode == "Masked")
            Material->BlendMode = BLEND_Masked;
        else if (BlendMode == "Translucent")
            Material->BlendMode = BLEND_Translucent;
        else if (BlendMode == "Additive")
            Material->BlendMode = BLEND_Additive;
        else if (BlendMode == "Modulate")
            Material->BlendMode = BLEND_Modulate;
        else if (BlendMode == "AlphaComposite")
            Material->BlendMode = BLEND_AlphaComposite;
        else if (BlendMode == "AlphaHoldout")
            Material->BlendMode = BLEND_AlphaHoldout;
        else
            bSuccess = false;
    }

    // Two Sided
    bool bTwoSided;
    if (Properties->TryGetBoolField(FStringView(TEXT("two_sided")), bTwoSided))
    {
        Material->TwoSided = bTwoSided;
    }

    // Dithered LOD Transition
    bool bDitheredLODTransition;
    if (Properties->TryGetBoolField(FStringView(TEXT("dithered_lod_transition")), bDitheredLODTransition))
    {
        Material->DitheredLODTransition = bDitheredLODTransition;
    }

    // Cast Contact Shadow
    bool bCastContactShadow;
    if (Properties->TryGetBoolField(FStringView(TEXT("cast_contact_shadow")), bCastContactShadow))
    {
        Material->bCastDynamicShadowAsMasked = bCastContactShadow;
    }

    // Base Color
    const TArray<TSharedPtr<FJsonValue>>* BaseColorArray = nullptr;
    if (Properties->TryGetArrayField(FStringView(TEXT("base_color")), BaseColorArray) && BaseColorArray && BaseColorArray->Num() == 4)
    {
        FLinearColor BaseColor(
            (*BaseColorArray)[0]->AsNumber(),
            (*BaseColorArray)[1]->AsNumber(),
            (*BaseColorArray)[2]->AsNumber(),
            (*BaseColorArray)[3]->AsNumber()
        );
        
        // Create a Vector4 constant expression for base color
        UMaterialExpressionVectorParameter* BaseColorParam = NewObject<UMaterialExpressionVectorParameter>(Material);
        BaseColorParam->ParameterName = TEXT("BaseColor");
        BaseColorParam->DefaultValue = BaseColor;
        Material->GetExpressionCollection().AddExpression(BaseColorParam);
        Material->GetEditorOnlyData()->BaseColor.Expression = BaseColorParam;
    }

    // Metallic
    double Metallic;
    if (Properties->TryGetNumberField(FStringView(TEXT("metallic")), Metallic))
    {
        // Create a scalar constant expression for metallic
        UMaterialExpressionScalarParameter* MetallicParam = NewObject<UMaterialExpressionScalarParameter>(Material);
        MetallicParam->ParameterName = TEXT("Metallic");
        MetallicParam->DefaultValue = FMath::Clamp(Metallic, 0.0, 1.0);
        Material->GetExpressionCollection().AddExpression(MetallicParam);
        Material->GetEditorOnlyData()->Metallic.Expression = MetallicParam;
    }

    // Roughness
    double Roughness;
    if (Properties->TryGetNumberField(FStringView(TEXT("roughness")), Roughness))
    {
        // Create a scalar constant expression for roughness
        UMaterialExpressionScalarParameter* RoughnessParam = NewObject<UMaterialExpressionScalarParameter>(Material);
        RoughnessParam->ParameterName = TEXT("Roughness");
        RoughnessParam->DefaultValue = FMath::Clamp(Roughness, 0.0, 1.0);
        Material->GetExpressionCollection().AddExpression(RoughnessParam);
        Material->GetEditorOnlyData()->Roughness.Expression = RoughnessParam;
    }

    return bSuccess;
}

//
// FMCPGetMaterialInfoHandler
//
TSharedPtr<FJsonObject> FMCPGetMaterialInfoHandler::Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    MCP_LOG_INFO("Handling get_material_info command");

    FString MaterialPath;
    if (!Params->TryGetStringField(FStringView(TEXT("path")), MaterialPath))
    {
        MCP_LOG_WARNING("Missing 'path' field in get_material_info command");
        return CreateErrorResponse("Missing 'path' field");
    }

    // Load the material
    UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Material)
    {
        MCP_LOG_ERROR("Failed to load material at path: %s", *MaterialPath);
        return CreateErrorResponse(FString::Printf(TEXT("Failed to load material at path: %s"), *MaterialPath));
    }

    // Get material info
    TSharedPtr<FJsonObject> ResultObj = GetMaterialInfo(Material);
    return CreateSuccessResponse(ResultObj);
}

TSharedPtr<FJsonObject> FMCPGetMaterialInfoHandler::GetMaterialInfo(UMaterial* Material)
{
    TSharedPtr<FJsonObject> Info = MakeShared<FJsonObject>();
    
    // Basic info
    Info->SetStringField("name", Material->GetName());
    Info->SetStringField("path", Material->GetPathName());

    // Shading Model
    FString ShadingModel = "Unknown";
    FMaterialShadingModelField ShadingModels = Material->GetShadingModels();
    if (ShadingModels.HasShadingModel(MSM_DefaultLit)) ShadingModel = "DefaultLit";
    else if (ShadingModels.HasShadingModel(MSM_Unlit)) ShadingModel = "Unlit";
    else if (ShadingModels.HasShadingModel(MSM_Subsurface)) ShadingModel = "Subsurface";
    else if (ShadingModels.HasShadingModel(MSM_PreintegratedSkin)) ShadingModel = "PreintegratedSkin";
    else if (ShadingModels.HasShadingModel(MSM_ClearCoat)) ShadingModel = "ClearCoat";
    else if (ShadingModels.HasShadingModel(MSM_SubsurfaceProfile)) ShadingModel = "SubsurfaceProfile";
    else if (ShadingModels.HasShadingModel(MSM_TwoSidedFoliage)) ShadingModel = "TwoSidedFoliage";
    else if (ShadingModels.HasShadingModel(MSM_Hair)) ShadingModel = "Hair";
    else if (ShadingModels.HasShadingModel(MSM_Cloth)) ShadingModel = "Cloth";
    else if (ShadingModels.HasShadingModel(MSM_Eye)) ShadingModel = "Eye";
    Info->SetStringField("shading_model", ShadingModel);

    // Blend Mode
    FString BlendMode;
    switch (Material->GetBlendMode())
    {
        case BLEND_Opaque: BlendMode = "Opaque"; break;
        case BLEND_Masked: BlendMode = "Masked"; break;
        case BLEND_Translucent: BlendMode = "Translucent"; break;
        case BLEND_Additive: BlendMode = "Additive"; break;
        case BLEND_Modulate: BlendMode = "Modulate"; break;
        case BLEND_AlphaComposite: BlendMode = "AlphaComposite"; break;
        case BLEND_AlphaHoldout: BlendMode = "AlphaHoldout"; break;
        default: BlendMode = "Unknown"; break;
    }
    Info->SetStringField("blend_mode", BlendMode);

    // Other properties
    Info->SetBoolField("two_sided", Material->IsTwoSided());
    Info->SetBoolField("dithered_lod_transition", Material->IsDitheredLODTransition());
    Info->SetBoolField("cast_contact_shadow", Material->bContactShadows);

    // Base Color
    TArray<TSharedPtr<FJsonValue>> BaseColorArray;
    FLinearColor BaseColorValue = FLinearColor::White;
    if (Material->GetEditorOnlyData()->BaseColor.Expression)
    {
        if (UMaterialExpressionVectorParameter* BaseColorParam = Cast<UMaterialExpressionVectorParameter>(Material->GetEditorOnlyData()->BaseColor.Expression))
        {
            BaseColorValue = BaseColorParam->DefaultValue;
        }
    }
    BaseColorArray.Add(MakeShared<FJsonValueNumber>(BaseColorValue.R));
    BaseColorArray.Add(MakeShared<FJsonValueNumber>(BaseColorValue.G));
    BaseColorArray.Add(MakeShared<FJsonValueNumber>(BaseColorValue.B));
    BaseColorArray.Add(MakeShared<FJsonValueNumber>(BaseColorValue.A));
    Info->SetArrayField("base_color", BaseColorArray);

    // Metallic
    float MetallicValue = 0.0f;
    if (Material->GetEditorOnlyData()->Metallic.Expression)
    {
        if (UMaterialExpressionScalarParameter* MetallicParam = Cast<UMaterialExpressionScalarParameter>(Material->GetEditorOnlyData()->Metallic.Expression))
        {
            MetallicValue = MetallicParam->DefaultValue;
        }
    }
    Info->SetNumberField("metallic", MetallicValue);

    // Roughness
    float RoughnessValue = 0.5f;
    if (Material->GetEditorOnlyData()->Roughness.Expression)
    {
        if (UMaterialExpressionScalarParameter* RoughnessParam = Cast<UMaterialExpressionScalarParameter>(Material->GetEditorOnlyData()->Roughness.Expression))
        {
            RoughnessValue = RoughnessParam->DefaultValue;
        }
    }
    Info->SetNumberField("roughness", RoughnessValue);

    return Info;
} 
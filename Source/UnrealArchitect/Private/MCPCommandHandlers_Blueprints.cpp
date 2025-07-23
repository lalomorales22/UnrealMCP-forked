#include "MCPCommandHandlers_Blueprints.h"
#include "MCPFileLogger.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"


//
// FMCPBlueprintUtils
//
TPair<UBlueprint*, bool> FMCPBlueprintUtils::CreateBlueprintAsset(
    const FString& PackagePath,
    const FString& BlueprintName,
    UClass* ParentClass)
{
    // Print debug information about the paths
    FString GameContentDir = FPaths::ProjectContentDir();
    FString PluginContentDir = FPaths::EnginePluginsDir() / TEXT("UnrealArchitect") / TEXT("Content");
    
    // Create the full path for the blueprint
    FString FullPackagePath = FString::Printf(TEXT("%s/%s"), *PackagePath, *BlueprintName);
    
    // Get the file paths
    FString DirectoryPath = FPackageName::LongPackageNameToFilename(PackagePath, TEXT(""));
    FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
    
    MCP_LOG_INFO("Creating blueprint asset:");
    MCP_LOG_INFO("  Package Path: %s", *PackagePath);
    MCP_LOG_INFO("  Blueprint Name: %s", *BlueprintName);
    MCP_LOG_INFO("  Full Package Path: %s", *FullPackagePath);
    MCP_LOG_INFO("  Directory Path: %s", *DirectoryPath);
    MCP_LOG_INFO("  Package File Name: %s", *PackageFileName);
    MCP_LOG_INFO("  Game Content Dir: %s", *GameContentDir);
    MCP_LOG_INFO("  Plugin Content Dir: %s", *PluginContentDir);
    
    // Additional logging for debugging
    FString AbsoluteGameDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    FString AbsoluteContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    FString AbsolutePackagePath = FPaths::ConvertRelativePathToFull(PackageFileName);
    
    MCP_LOG_INFO("  Absolute Game Dir: %s", *AbsoluteGameDir);
    MCP_LOG_INFO("  Absolute Content Dir: %s", *AbsoluteContentDir);
    MCP_LOG_INFO("  Absolute Package Path: %s", *AbsolutePackagePath);
    
    // Ensure the directory exists
    IFileManager::Get().MakeDirectory(*DirectoryPath, true);
    
    // Verify directory was created
    if (IFileManager::Get().DirectoryExists(*DirectoryPath))
    {
        MCP_LOG_INFO("  Directory exists or was created successfully: %s", *DirectoryPath);
    }
    else
    {
        MCP_LOG_ERROR("  Failed to create directory: %s", *DirectoryPath);
    }

    // Check if a blueprint with this name already exists in the package
    UBlueprint* ExistingBlueprint = LoadObject<UBlueprint>(nullptr, *FullPackagePath);
    if (ExistingBlueprint)
    {
        MCP_LOG_WARNING("Blueprint already exists at path: %s", *FullPackagePath);
        return TPair<UBlueprint*, bool>(ExistingBlueprint, true);
    }

    // Create or load the package for the full path
    UPackage* Package = CreatePackage(*FullPackagePath);
    if (!Package)
    {
        MCP_LOG_ERROR("Failed to create package for blueprint");
        return TPair<UBlueprint*, bool>(nullptr, false);
    }

    Package->FullyLoad();

    // Create the Blueprint
    UBlueprint* NewBlueprint = nullptr;
    
    // Use a try-catch block to handle potential errors in CreateBlueprint
    try
    {
        NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
            ParentClass,
            Package,
            FName(*BlueprintName),
            BPTYPE_Normal,
            UBlueprint::StaticClass(),
            UBlueprintGeneratedClass::StaticClass()
        );
    }
    catch (const std::exception& e)
    {
        MCP_LOG_ERROR("Exception while creating blueprint: %s", ANSI_TO_TCHAR(e.what()));
        return TPair<UBlueprint*, bool>(nullptr, false);
    }
    catch (...)
    {
        MCP_LOG_ERROR("Unknown exception while creating blueprint");
        return TPair<UBlueprint*, bool>(nullptr, false);
    }

    if (!NewBlueprint)
    {
        MCP_LOG_ERROR("Failed to create blueprint");
        return TPair<UBlueprint*, bool>(nullptr, false);
    }

    // Save the package
    Package->MarkPackageDirty();
    MCP_LOG_INFO("  Saving package to: %s", *PackageFileName);
    
    // Use the new SavePackage API
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    bool bSaveSuccess = UPackage::SavePackage(Package, NewBlueprint, *PackageFileName, SaveArgs);
    
    if (bSaveSuccess)
    {
        MCP_LOG_INFO("  Package saved successfully to: %s", *PackageFileName);
        
        // Check if the file actually exists
        if (IFileManager::Get().FileExists(*PackageFileName))
        {
            MCP_LOG_INFO("  File exists at: %s", *PackageFileName);
        }
        else
        {
            MCP_LOG_ERROR("  File does NOT exist at: %s", *PackageFileName);
        }
    }
    else
    {
        MCP_LOG_ERROR("  Failed to save package to: %s", *PackageFileName);
    }

    // Notify the asset registry
    FAssetRegistryModule::AssetCreated(NewBlueprint);

    return TPair<UBlueprint*, bool>(NewBlueprint, true);
}

TPair<UK2Node_Event*, bool> FMCPBlueprintUtils::AddEventNode(
    UBlueprint* Blueprint,
    const FString& EventName,
    UClass* ParentClass)
{
    if (!Blueprint)
    {
        return TPair<UK2Node_Event*, bool>(nullptr, false);
    }

    // Find or create the event graph
    UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!EventGraph)
    {
        EventGraph = FBlueprintEditorUtils::CreateNewGraph(
            Blueprint,
            FName("EventGraph"),
            UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass()
        );
        Blueprint->UbergraphPages.Add(EventGraph);
    }

    // Create the custom event node
    UK2Node_Event* EventNode = NewObject<UK2Node_Event>(EventGraph);
    EventNode->EventReference.SetExternalMember(FName(*EventName), ParentClass);
    EventNode->bOverrideFunction = true;
    EventNode->AllocateDefaultPins();
    EventGraph->Nodes.Add(EventNode);

    return TPair<UK2Node_Event*, bool>(EventNode, true);
}

TPair<UK2Node_CallFunction*, bool> FMCPBlueprintUtils::AddPrintStringNode(
    UEdGraph* Graph,
    const FString& Message)
{
    if (!Graph)
    {
        return TPair<UK2Node_CallFunction*, bool>(nullptr, false);
    }

    // Create print string node
    UK2Node_CallFunction* PrintNode = NewObject<UK2Node_CallFunction>(Graph);
    PrintNode->FunctionReference.SetExternalMember(FName("PrintString"), UKismetSystemLibrary::StaticClass());
    PrintNode->AllocateDefaultPins();
    Graph->Nodes.Add(PrintNode);

    // Set the string input
    UEdGraphPin* StringPin = PrintNode->FindPinChecked(FName("InString"));
    StringPin->DefaultValue = Message;

    return TPair<UK2Node_CallFunction*, bool>(PrintNode, true);
}

//
// FMCPCreateBlueprintHandler
//
TSharedPtr<FJsonObject> FMCPCreateBlueprintHandler::Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    MCP_LOG_INFO("Handling create_blueprint command");

    FString PackagePath;
    if (!Params->TryGetStringField(TEXT("package_path"), PackagePath))
    {
        MCP_LOG_WARNING("Missing 'package_path' field in create_blueprint command");
        return CreateErrorResponse("Missing 'package_path' field");
    }

    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("name"), BlueprintName))
    {
        MCP_LOG_WARNING("Missing 'name' field in create_blueprint command");
        return CreateErrorResponse("Missing 'name' field");
    }

    // Get optional properties
    const TSharedPtr<FJsonObject>* Properties = nullptr;
    Params->TryGetObjectField(TEXT("properties"), Properties);

    // Create the blueprint
    TPair<UBlueprint*, bool> Result = CreateBlueprint(PackagePath, BlueprintName, Properties ? *Properties : nullptr);

    if (Result.Value)
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField("name", Result.Key->GetName());
        ResultObj->SetStringField("path", Result.Key->GetPathName());

        FString FilePath = Result.Key->GetOutermost()->GetMetaData(TEXT("MCPFilePath"));
        if (!FilePath.IsEmpty())
        {
            ResultObj->SetStringField("file_path", FilePath);
        }

        return CreateSuccessResponse(ResultObj);
    }
    else
    {
        return CreateErrorResponse("Failed to create blueprint");
    }
}

TPair<UBlueprint*, bool> FMCPCreateBlueprintHandler::CreateBlueprint(
    const FString& PackagePath,
    const FString& BlueprintName,
    const TSharedPtr<FJsonObject>& Properties)
{
    // Ensure the package path is correctly formatted
    // We need to create a proper directory structure
    FString DirectoryPath;
    FString AssetName;
    
    // Create a proper directory structure
    // For example, if PackagePath is "/Game/Blueprints" and BlueprintName is "TestBlueprint",
    // we want to create a directory at "/Game/Blueprints" and place "TestBlueprint" inside it
    DirectoryPath = PackagePath;
    AssetName = BlueprintName;
    
    // Create the full path for the blueprint
    FString FullPackagePath = FString::Printf(TEXT("%s/%s"), *DirectoryPath, *AssetName);
    MCP_LOG_INFO("Creating blueprint at path: %s", *FullPackagePath);
    
    // Check if a blueprint with this name already exists
    UBlueprint* ExistingBlueprint = LoadObject<UBlueprint>(nullptr, *FullPackagePath);
    if (ExistingBlueprint)
    {
        MCP_LOG_WARNING("Blueprint already exists at path: %s", *FullPackagePath);
        return TPair<UBlueprint*, bool>(ExistingBlueprint, true);
    }

    // Default to Actor as parent class
    UClass* ParentClass = AActor::StaticClass();

    // Check if a different parent class is specified
    if (Properties.IsValid())
    {
        FString ParentClassName;
        if (Properties->TryGetStringField(TEXT("parent_class"), ParentClassName))
        {
            // First try to find the class using its full path
            UClass* FoundClass = LoadObject<UClass>(nullptr, *ParentClassName);
            
            // If not found with direct path, try to find it in common class paths
            if (!FoundClass)
            {
                // Try with /Script/Engine path (for engine classes)
                FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *ParentClassName);
                FoundClass = LoadObject<UClass>(nullptr, *EnginePath);
                
                // If still not found, try with game's path
                if (!FoundClass)
                {
                    FString GamePath = FString::Printf(TEXT("/Script/%s.%s"), 
                        FApp::GetProjectName(), 
                        *ParentClassName);
                    FoundClass = LoadObject<UClass>(nullptr, *GamePath);
                }
            }

            if (FoundClass)
            {
                ParentClass = FoundClass;
            }
            else
            {
                MCP_LOG_WARNING("Could not find parent class '%s', using default Actor class", *ParentClassName);
            }
        }
    }

    // Create the blueprint directly in the specified directory
    TPair<UBlueprint*, bool> Result = FMCPBlueprintUtils::CreateBlueprintAsset(DirectoryPath, AssetName, ParentClass);

    if (Result.Value && Result.Key)
    {
        FString PackageFileName = FPackageName::LongPackageNameToFilename(FullPackagePath, FPackageName::GetAssetPackageExtension());
        Result.Key->GetOutermost()->SetMetaData(TEXT("MCPFilePath"), *PackageFileName);
    }

    return Result;
}

//
// FMCPModifyBlueprintHandler
//
TSharedPtr<FJsonObject> FMCPModifyBlueprintHandler::Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    MCP_LOG_INFO("Handling modify_blueprint command");

    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        MCP_LOG_WARNING("Missing 'blueprint_path' field in modify_blueprint command");
        return CreateErrorResponse("Missing 'blueprint_path' field");
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint at path: %s"), *BlueprintPath));
    }

    // Get properties to modify
    const TSharedPtr<FJsonObject>* Properties = nullptr;
    if (!Params->TryGetObjectField(TEXT("properties"), Properties) || !Properties)
    {
        return CreateErrorResponse("Missing 'properties' field");
    }

    if (ModifyBlueprint(Blueprint, *Properties))
    {
        return CreateSuccessResponse();
    }
    else
    {
        return CreateErrorResponse("Failed to modify blueprint");
    }
}

bool FMCPModifyBlueprintHandler::ModifyBlueprint(UBlueprint* Blueprint, const TSharedPtr<FJsonObject>& Properties)
{
    if (!Blueprint || !Properties.IsValid())
    {
        return false;
    }

    bool bModified = false;

    // Handle blueprint description
    FString Description;
    if (Properties->TryGetStringField(TEXT("description"), Description))
    {
        Blueprint->BlueprintDescription = Description;
        bModified = true;
    }

    // Handle blueprint category
    FString Category;
    if (Properties->TryGetStringField(TEXT("category"), Category))
    {
        Blueprint->BlueprintCategory = Category;
        bModified = true;
    }

    // Handle parent class change
    FString ParentClassName;
    if (Properties->TryGetStringField(TEXT("parent_class"), ParentClassName))
    {
        // First try to find the class using its full path
        UClass* FoundClass = LoadObject<UClass>(nullptr, *ParentClassName);
        
        // If not found with direct path, try to find it in common class paths
        if (!FoundClass)
        {
            // Try with /Script/Engine path (for engine classes)
            FString EnginePath = FString::Printf(TEXT("/Script/Engine.%s"), *ParentClassName);
            FoundClass = LoadObject<UClass>(nullptr, *EnginePath);
            
            // If still not found, try with game's path
            if (!FoundClass)
            {
                FString GamePath = FString::Printf(TEXT("/Script/%s.%s"), 
                    FApp::GetProjectName(), 
                    *ParentClassName);
                FoundClass = LoadObject<UClass>(nullptr, *GamePath);
            }
        }

        if (FoundClass)
        {
            Blueprint->ParentClass = FoundClass;
            bModified = true;
        }
        else
        {
            MCP_LOG_WARNING("Could not find parent class '%s' for blueprint modification", *ParentClassName);
        }
    }

    // Handle additional categories to hide
    const TSharedPtr<FJsonObject>* Options = nullptr;
    if (Properties->TryGetObjectField(TEXT("options"), Options) && Options)
    {
        // Handle hide categories
        const TArray<TSharedPtr<FJsonValue>>* HideCategories = nullptr;
        if ((*Options)->TryGetArrayField(TEXT("hide_categories"), HideCategories) && HideCategories)
        {
            for (const TSharedPtr<FJsonValue>& Value : *HideCategories)
            {
                FString CategoryName;
                if (Value->TryGetString(CategoryName) && !CategoryName.IsEmpty())
                {
                    Blueprint->HideCategories.AddUnique(CategoryName);
                    bModified = true;
                }
            }
        }
        
        // Handle namespace
        FString Namespace;
        if ((*Options)->TryGetStringField(TEXT("namespace"), Namespace))
        {
            Blueprint->BlueprintNamespace = Namespace;
            bModified = true;
        }
        
        // Handle display name
        FString DisplayName;
        if ((*Options)->TryGetStringField(TEXT("display_name"), DisplayName))
        {
            Blueprint->BlueprintDisplayName = DisplayName;
            bModified = true;
        }
        
        // Handle compile mode
        FString CompileMode;
        if ((*Options)->TryGetStringField(TEXT("compile_mode"), CompileMode))
        {
            if (CompileMode.Equals(TEXT("Default"), ESearchCase::IgnoreCase))
            {
                Blueprint->CompileMode = EBlueprintCompileMode::Default;
                bModified = true;
            }
            else if (CompileMode.Equals(TEXT("Development"), ESearchCase::IgnoreCase))
            {
                Blueprint->CompileMode = EBlueprintCompileMode::Development;
                bModified = true;
            }
            else if (CompileMode.Equals(TEXT("FinalRelease"), ESearchCase::IgnoreCase))
            {
                Blueprint->CompileMode = EBlueprintCompileMode::FinalRelease;
                bModified = true;
            }
        }
        
        // Handle class options
        bool bGenerateAbstractClass = false;
        if ((*Options)->TryGetBoolField(TEXT("abstract_class"), bGenerateAbstractClass))
        {
            Blueprint->bGenerateAbstractClass = bGenerateAbstractClass;
            bModified = true;
        }
        
        bool bGenerateConstClass = false;
        if ((*Options)->TryGetBoolField(TEXT("const_class"), bGenerateConstClass))
        {
            Blueprint->bGenerateConstClass = bGenerateConstClass;
            bModified = true;
        }
        
        bool bDeprecate = false;
        if ((*Options)->TryGetBoolField(TEXT("deprecate"), bDeprecate))
        {
            Blueprint->bDeprecate = bDeprecate;
            bModified = true;
        }
    }

    if (bModified)
    {
        // Mark the package as dirty
        Blueprint->MarkPackageDirty();
        
        // Recompile the blueprint if it was modified
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        
        // Save the package
        UPackage* Package = Blueprint->GetOutermost();
        if (Package)
        {
            FString PackagePath = Package->GetName();
            FString SavePackageFileName = FPackageName::LongPackageNameToFilename(
                PackagePath, 
                FPackageName::GetAssetPackageExtension()
            );
            
            // Use the new SavePackage API
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            SaveArgs.SaveFlags = SAVE_NoError;
            UPackage::SavePackage(Package, Blueprint, *SavePackageFileName, SaveArgs);
        }
    }

    return bModified;
}

//
// FMCPGetBlueprintInfoHandler
//
TSharedPtr<FJsonObject> FMCPGetBlueprintInfoHandler::Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    MCP_LOG_INFO("Handling get_blueprint_info command");

    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        MCP_LOG_WARNING("Missing 'blueprint_path' field in get_blueprint_info command");
        return CreateErrorResponse("Missing 'blueprint_path' field");
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        return CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint at path: %s"), *BlueprintPath));
    }

    return CreateSuccessResponse(GetBlueprintInfo(Blueprint));
}

TSharedPtr<FJsonObject> FMCPGetBlueprintInfoHandler::GetBlueprintInfo(UBlueprint* Blueprint)
{
    TSharedPtr<FJsonObject> Info = MakeShared<FJsonObject>();
    if (!Blueprint)
    {
        return Info;
    }

    Info->SetStringField("name", Blueprint->GetName());
    Info->SetStringField("path", Blueprint->GetPathName());
    Info->SetStringField("parent_class", Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : TEXT("None"));
    
    // Add blueprint-specific properties
    Info->SetStringField("category", Blueprint->BlueprintCategory);
    Info->SetStringField("description", Blueprint->BlueprintDescription);
    Info->SetStringField("display_name", Blueprint->BlueprintDisplayName);
    Info->SetStringField("namespace", Blueprint->BlueprintNamespace);
    
    // Add blueprint type
    FString BlueprintTypeStr;
    switch (Blueprint->BlueprintType)
    {
    case BPTYPE_Normal:
        BlueprintTypeStr = TEXT("Normal");
        break;
    case BPTYPE_Const:
        BlueprintTypeStr = TEXT("Const");
        break;
    case BPTYPE_MacroLibrary:
        BlueprintTypeStr = TEXT("MacroLibrary");
        break;
    case BPTYPE_Interface:
        BlueprintTypeStr = TEXT("Interface");
        break;
    case BPTYPE_LevelScript:
        BlueprintTypeStr = TEXT("LevelScript");
        break;
    case BPTYPE_FunctionLibrary:
        BlueprintTypeStr = TEXT("FunctionLibrary");
        break;
    default:
        BlueprintTypeStr = TEXT("Unknown");
        break;
    }
    Info->SetStringField("blueprint_type", BlueprintTypeStr);
    
    // Add class options
    TSharedPtr<FJsonObject> ClassOptions = MakeShared<FJsonObject>();
    ClassOptions->SetBoolField("abstract_class", Blueprint->bGenerateAbstractClass);
    ClassOptions->SetBoolField("const_class", Blueprint->bGenerateConstClass);
    ClassOptions->SetBoolField("deprecated", Blueprint->bDeprecate);
    
    // Add compile mode
    FString CompileModeStr;
    switch (Blueprint->CompileMode)
    {
    case EBlueprintCompileMode::Default:
        CompileModeStr = TEXT("Default");
        break;
    case EBlueprintCompileMode::Development:
        CompileModeStr = TEXT("Development");
        break;
    case EBlueprintCompileMode::FinalRelease:
        CompileModeStr = TEXT("FinalRelease");
        break;
    default:
        CompileModeStr = TEXT("Unknown");
        break;
    }
    ClassOptions->SetStringField("compile_mode", CompileModeStr);
    
    // Add hide categories
    TArray<TSharedPtr<FJsonValue>> HideCategories;
    for (const FString& Category : Blueprint->HideCategories)
    {
        HideCategories.Add(MakeShared<FJsonValueString>(Category));
    }
    ClassOptions->SetArrayField("hide_categories", HideCategories);
    
    Info->SetObjectField("class_options", ClassOptions);

    // Add information about functions
    TArray<TSharedPtr<FJsonValue>> Functions;
    for (UEdGraph* FuncGraph : Blueprint->FunctionGraphs)
    {
        TSharedPtr<FJsonObject> FuncInfo = MakeShared<FJsonObject>();
        FuncInfo->SetStringField("name", FuncGraph->GetName());
        Functions.Add(MakeShared<FJsonValueObject>(FuncInfo));
    }
    Info->SetArrayField("functions", Functions);

    // Add information about events
    TArray<TSharedPtr<FJsonValue>> Events;
    UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (EventGraph)
    {
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
            {
                TSharedPtr<FJsonObject> EventInfo = MakeShared<FJsonObject>();
                EventInfo->SetStringField("name", EventNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
                Events.Add(MakeShared<FJsonValueObject>(EventInfo));
            }
        }
    }
    Info->SetArrayField("events", Events);

    return Info;
}

//
// FMCPCreateBlueprintEventHandler
//
TSharedPtr<FJsonObject> FMCPCreateBlueprintEventHandler::Execute(const TSharedPtr<FJsonObject>& Params, FSocket* ClientSocket)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return CreateErrorResponse("Invalid World context");
    }

    // Get event name
    FString EventName;
    if (!Params->TryGetStringField(TEXT("event_name"), EventName))
    {
        return CreateErrorResponse("Missing 'event_name' field");
    }

    // Get blueprint path
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        // If no blueprint path is provided, create a new blueprint
        BlueprintPath = FString::Printf(TEXT("/Game/GeneratedBlueprints/BP_MCP_%s"), *EventName);
    }

    // Get optional event parameters
    const TSharedPtr<FJsonObject>* EventParamsPtr = nullptr;
    Params->TryGetObjectField(TEXT("parameters"), EventParamsPtr);
    TSharedPtr<FJsonObject> EventParams = EventParamsPtr ? *EventParamsPtr : nullptr;

    // Create the blueprint event
    TPair<bool, TSharedPtr<FJsonObject>> Result = CreateBlueprintEvent(World, EventName, BlueprintPath, EventParams);
    
    if (Result.Key)
    {
        return CreateSuccessResponse(Result.Value);
    }
    else
    {
        return CreateErrorResponse("Failed to create blueprint event");
    }
}

TPair<bool, TSharedPtr<FJsonObject>> FMCPCreateBlueprintEventHandler::CreateBlueprintEvent(
    UWorld* World,
    const FString& EventName,
    const FString& BlueprintPath,
    const TSharedPtr<FJsonObject>& EventParameters)
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    // Try to load existing blueprint or create new one
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        // Create new blueprint
        FString PackagePath = FPackageName::GetLongPackagePath(BlueprintPath);
        FString BlueprintName = FPackageName::GetShortName(BlueprintPath);
        
        TPair<UBlueprint*, bool> BlueprintResult = FMCPBlueprintUtils::CreateBlueprintAsset(PackagePath, BlueprintName, AActor::StaticClass());
        if (!BlueprintResult.Value || !BlueprintResult.Key)
        {
            MCP_LOG_ERROR("Failed to create blueprint asset");
            return TPair<bool, TSharedPtr<FJsonObject>>(false, nullptr);
        }
        Blueprint = BlueprintResult.Key;
    }

    // Add the event node
    TPair<UK2Node_Event*, bool> EventNodeResult = FMCPBlueprintUtils::AddEventNode(Blueprint, EventName, AActor::StaticClass());
    if (!EventNodeResult.Value || !EventNodeResult.Key)
    {
        MCP_LOG_ERROR("Failed to add event node");
        return TPair<bool, TSharedPtr<FJsonObject>>(false, nullptr);
    }

    // Add a print string node for testing
    UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (EventGraph)
    {
        TPair<UK2Node_CallFunction*, bool> PrintNodeResult = FMCPBlueprintUtils::AddPrintStringNode(
            EventGraph,
            FString::Printf(TEXT("Event '%s' triggered!"), *EventName)
        );

        if (PrintNodeResult.Value && PrintNodeResult.Key)
        {
            // Connect the event to the print node
            UEdGraphPin* EventThenPin = EventNodeResult.Key->FindPinChecked(UEdGraphSchema_K2::PN_Then);
            UEdGraphPin* PrintExecPin = PrintNodeResult.Key->FindPinChecked(UEdGraphSchema_K2::PN_Execute);
            EventGraph->GetSchema()->TryCreateConnection(EventThenPin, PrintExecPin);
        }
    }

    // Compile and save the blueprint
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    
    Result->SetStringField("blueprint", Blueprint->GetName());
    Result->SetStringField("event", EventName);
    Result->SetStringField("path", Blueprint->GetPathName());
    
    return TPair<bool, TSharedPtr<FJsonObject>>(true, Result);
} 

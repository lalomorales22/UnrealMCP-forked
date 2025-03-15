#include "MCPCommandHandlers.h"

#include "ActorEditorUtils.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "MCPFileLogger.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "MCPConstants.h"

// Shorthand for logger
#define MCP_LOG(Verbosity, Format, ...) FMCPFileLogger::Get().Log(ELogVerbosity::Verbosity, FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_INFO(Format, ...) FMCPFileLogger::Get().Info(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_ERROR(Format, ...) FMCPFileLogger::Get().Error(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_WARNING(Format, ...) FMCPFileLogger::Get().Warning(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_VERBOSE(Format, ...) FMCPFileLogger::Get().Verbose(FString::Printf(TEXT(Format), ##__VA_ARGS__))

//
// FMCPGetSceneInfoHandler
//
TSharedPtr<FJsonObject> FMCPGetSceneInfoHandler::Execute(const TSharedPtr<FJsonObject> &Params, FSocket *ClientSocket)
{
    MCP_LOG_INFO("Handling get_scene_info command");

    UWorld *World = GEditor->GetEditorWorldContext().World();
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> ActorsArray;

    int32 ActorCount = 0;
    int32 TotalActorCount = 0;
    bool bLimitReached = false;
    
    // First count the total number of actors
    for (TActorIterator<AActor> CountIt(World); CountIt; ++CountIt)
    {
        TotalActorCount++;
    }
    
    // Then collect actor info up to the limit
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor *Actor = *It;
        TSharedPtr<FJsonObject> ActorInfo = MakeShared<FJsonObject>();
        ActorInfo->SetStringField("name", Actor->GetName());
        ActorInfo->SetStringField("type", Actor->GetClass()->GetName());
        
        // Add the actor label (user-facing friendly name)
        ActorInfo->SetStringField("label", Actor->GetActorLabel());

        // Add location
        FVector Location = Actor->GetActorLocation();
        TArray<TSharedPtr<FJsonValue>> LocationArray;
        LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
        LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
        LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
        ActorInfo->SetArrayField("location", LocationArray);

        ActorsArray.Add(MakeShared<FJsonValueObject>(ActorInfo));
        ActorCount++;
        if (ActorCount >= MCPConstants::MAX_ACTORS_IN_SCENE_INFO)
        {
            bLimitReached = true;
            MCP_LOG_WARNING("Actor limit reached (%d). Only returning %d of %d actors.", 
                MCPConstants::MAX_ACTORS_IN_SCENE_INFO, ActorCount, TotalActorCount);
            break; // Limit for performance
        }
    }

    Result->SetStringField("level", World->GetName());
    Result->SetNumberField("actor_count", TotalActorCount);
    Result->SetNumberField("returned_actor_count", ActorCount);
    Result->SetBoolField("limit_reached", bLimitReached);
    Result->SetArrayField("actors", ActorsArray);

    MCP_LOG_INFO("Sending get_scene_info response with %d/%d actors", ActorCount, TotalActorCount);

    return CreateSuccessResponse(Result);
}

//
// FMCPCreateObjectHandler
//
TSharedPtr<FJsonObject> FMCPCreateObjectHandler::Execute(const TSharedPtr<FJsonObject> &Params, FSocket *ClientSocket)
{
    UWorld *World = GEditor->GetEditorWorldContext().World();

    FString Type;
    if (!Params->TryGetStringField(FStringView(TEXT("type")), Type))
    {
        MCP_LOG_WARNING("Missing 'type' field in create_object command");
        return CreateErrorResponse("Missing 'type' field");
    }

    // Get location
    const TArray<TSharedPtr<FJsonValue>> *LocationArrayPtr = nullptr;
    if (!Params->TryGetArrayField(FStringView(TEXT("location")), LocationArrayPtr) || !LocationArrayPtr || LocationArrayPtr->Num() != 3)
    {
        MCP_LOG_WARNING("Invalid 'location' field in create_object command");
        return CreateErrorResponse("Invalid 'location' field");
    }

    FVector Location(
        (*LocationArrayPtr)[0]->AsNumber(),
        (*LocationArrayPtr)[1]->AsNumber(),
        (*LocationArrayPtr)[2]->AsNumber());

    // Convert type to lowercase for case-insensitive comparison
    FString TypeLower = Type.ToLower();

    if (Type == "StaticMeshActor")
    {
        // Get mesh path if specified
        FString MeshPath;
        Params->TryGetStringField(FStringView(TEXT("mesh")), MeshPath);

        // Get label if specified
        FString Label;
        Params->TryGetStringField(FStringView(TEXT("label")), Label);

        // Create the actor
        TPair<AStaticMeshActor *, bool> Result = CreateStaticMeshActor(World, Location, MeshPath, Label);

        if (Result.Value)
        {
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetStringField("name", Result.Key->GetName());
            ResultObj->SetStringField("label", Result.Key->GetActorLabel());
            return CreateSuccessResponse(ResultObj);
        }
        else
        {
            return CreateErrorResponse("Failed to create StaticMeshActor");
        }
    }
    else if (TypeLower == "cube")
    {
        // Create a cube actor
        FString Label;
        Params->TryGetStringField(FStringView(TEXT("label")), Label);
        TPair<AStaticMeshActor *, bool> Result = CreateCubeActor(World, Location, Label);

        if (Result.Value)
        {
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetStringField("name", Result.Key->GetName());
            ResultObj->SetStringField("label", Result.Key->GetActorLabel());
            return CreateSuccessResponse(ResultObj);
        }
        else
        {
            return CreateErrorResponse("Failed to create cube");
        }
    }
    else
    {
        MCP_LOG_WARNING("Unsupported actor type: %s", *Type);
        return CreateErrorResponse(FString::Printf(TEXT("Unsupported actor type: %s"), *Type));
    }
}

TPair<AStaticMeshActor *, bool> FMCPCreateObjectHandler::CreateStaticMeshActor(UWorld *World, const FVector &Location, const FString &MeshPath, const FString &Label)
{
    if (!World)
    {
        return TPair<AStaticMeshActor *, bool>(nullptr, false);
    }

    // Create the actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = NAME_None; // Auto-generate a name
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AStaticMeshActor *NewActor = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, SpawnParams);
    if (NewActor)
    {
        MCP_LOG_INFO("Created StaticMeshActor at location (%f, %f, %f)", Location.X, Location.Y, Location.Z);

        // Set mesh if specified
        if (!MeshPath.IsEmpty())
        {
            UStaticMesh *Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
            if (Mesh)
            {
                NewActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
                MCP_LOG_INFO("Set mesh to %s", *MeshPath);
            }
            else
            {
                MCP_LOG_WARNING("Failed to load mesh %s", *MeshPath);
            }
        }

        // Set a descriptive label
        if (!Label.IsEmpty())
        {
            NewActor->SetActorLabel(Label);
            MCP_LOG_INFO("Set custom label to %s", *Label);
        }
        else
        {
            NewActor->SetActorLabel(FString::Printf(TEXT("MCP_StaticMesh_%d"), FMath::RandRange(1000, 9999)));
        }

        return TPair<AStaticMeshActor *, bool>(NewActor, true);
    }
    else
    {
        MCP_LOG_ERROR("Failed to create StaticMeshActor");
        return TPair<AStaticMeshActor *, bool>(nullptr, false);
    }
}

TPair<AStaticMeshActor *, bool> FMCPCreateObjectHandler::CreateCubeActor(UWorld *World, const FVector &Location, const FString &Label)
{
    if (!World)
    {
        return TPair<AStaticMeshActor *, bool>(nullptr, false);
    }

    // Create a StaticMeshActor with a cube mesh
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = NAME_None;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AStaticMeshActor *NewActor = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, SpawnParams);
    if (NewActor)
    {
        MCP_LOG_INFO("Created Cube at location (%f, %f, %f)", Location.X, Location.Y, Location.Z);

        // Set cube mesh
        UStaticMesh *CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (CubeMesh)
        {
            NewActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
            MCP_LOG_INFO("Set cube mesh");

            // Set a descriptive label
            if (!Label.IsEmpty())
            {
                NewActor->SetActorLabel(Label);
                MCP_LOG_INFO("Set custom label to %s", *Label);
            }
            else
            {
                NewActor->SetActorLabel(FString::Printf(TEXT("MCP_Cube_%d"), FMath::RandRange(1000, 9999)));
            }

            return TPair<AStaticMeshActor *, bool>(NewActor, true);
        }
        else
        {
            MCP_LOG_WARNING("Failed to load cube mesh");
            World->DestroyActor(NewActor);
            return TPair<AStaticMeshActor *, bool>(nullptr, false);
        }
    }
    else
    {
        MCP_LOG_ERROR("Failed to create Cube");
        return TPair<AStaticMeshActor *, bool>(nullptr, false);
    }
}

//
// FMCPModifyObjectHandler
//
TSharedPtr<FJsonObject> FMCPModifyObjectHandler::Execute(const TSharedPtr<FJsonObject> &Params, FSocket *ClientSocket)
{
    UWorld *World = GEditor->GetEditorWorldContext().World();

    FString ActorName;
    if (!Params->TryGetStringField(FStringView(TEXT("name")), ActorName))
    {
        MCP_LOG_WARNING("Missing 'name' field in modify_object command");
        return CreateErrorResponse("Missing 'name' field");
    }

    AActor *Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }

    if (!Actor)
    {
        MCP_LOG_WARNING("Actor not found: %s", *ActorName);
        return CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    bool bModified = false;

    // Check for location update
    const TArray<TSharedPtr<FJsonValue>> *LocationArrayPtr = nullptr;
    if (Params->TryGetArrayField(FStringView(TEXT("location")), LocationArrayPtr) && LocationArrayPtr && LocationArrayPtr->Num() == 3)
    {
        FVector NewLocation(
            (*LocationArrayPtr)[0]->AsNumber(),
            (*LocationArrayPtr)[1]->AsNumber(),
            (*LocationArrayPtr)[2]->AsNumber());

        Actor->SetActorLocation(NewLocation);
        MCP_LOG_INFO("Updated location of %s to (%f, %f, %f)", *ActorName, NewLocation.X, NewLocation.Y, NewLocation.Z);
        bModified = true;
    }

    // Check for rotation update
    const TArray<TSharedPtr<FJsonValue>> *RotationArrayPtr = nullptr;
    if (Params->TryGetArrayField(FStringView(TEXT("rotation")), RotationArrayPtr) && RotationArrayPtr && RotationArrayPtr->Num() == 3)
    {
        FRotator NewRotation(
            (*RotationArrayPtr)[0]->AsNumber(),
            (*RotationArrayPtr)[1]->AsNumber(),
            (*RotationArrayPtr)[2]->AsNumber());

        Actor->SetActorRotation(NewRotation);
        MCP_LOG_INFO("Updated rotation of %s to (%f, %f, %f)", *ActorName, NewRotation.Pitch, NewRotation.Yaw, NewRotation.Roll);
        bModified = true;
    }

    // Check for scale update
    const TArray<TSharedPtr<FJsonValue>> *ScaleArrayPtr = nullptr;
    if (Params->TryGetArrayField(FStringView(TEXT("scale")), ScaleArrayPtr) && ScaleArrayPtr && ScaleArrayPtr->Num() == 3)
    {
        FVector NewScale(
            (*ScaleArrayPtr)[0]->AsNumber(),
            (*ScaleArrayPtr)[1]->AsNumber(),
            (*ScaleArrayPtr)[2]->AsNumber());

        Actor->SetActorScale3D(NewScale);
        MCP_LOG_INFO("Updated scale of %s to (%f, %f, %f)", *ActorName, NewScale.X, NewScale.Y, NewScale.Z);
        bModified = true;
    }

    if (bModified)
    {
        // Create a result object with the actor name
        TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetStringField("name", Actor->GetName());

        // Return success with the result object
        return CreateSuccessResponse(Result);
    }
    else
    {
        MCP_LOG_WARNING("No modifications specified for %s", *ActorName);
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "warning");
        Response->SetStringField("message", "No modifications specified");
        return Response;
    }
}

//
// FMCPDeleteObjectHandler
//
TSharedPtr<FJsonObject> FMCPDeleteObjectHandler::Execute(const TSharedPtr<FJsonObject> &Params, FSocket *ClientSocket)
{
    UWorld *World = GEditor->GetEditorWorldContext().World();

    FString ActorName;
    if (!Params->TryGetStringField(FStringView(TEXT("name")), ActorName))
    {
        MCP_LOG_WARNING("Missing 'name' field in delete_object command");
        return CreateErrorResponse("Missing 'name' field");
    }

    AActor *Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }

    if (!Actor)
    {
        MCP_LOG_WARNING("Actor not found: %s", *ActorName);
        return CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Check if the actor can be deleted
    if (!FActorEditorUtils::IsABuilderBrush(Actor))
    {
        bool bDestroyed = World->DestroyActor(Actor);
        if (bDestroyed)
        {
            MCP_LOG_INFO("Deleted actor: %s", *ActorName);
            return CreateSuccessResponse();
        }
        else
        {
            MCP_LOG_ERROR("Failed to delete actor: %s", *ActorName);
            return CreateErrorResponse(FString::Printf(TEXT("Failed to delete actor: %s"), *ActorName));
        }
    }
    else
    {
        MCP_LOG_WARNING("Cannot delete special actor: %s", *ActorName);
        return CreateErrorResponse(FString::Printf(TEXT("Cannot delete special actor: %s"), *ActorName));
    }
}

//
// FMCPExecutePythonHandler
//
TSharedPtr<FJsonObject> FMCPExecutePythonHandler::Execute(const TSharedPtr<FJsonObject> &Params, FSocket *ClientSocket)
{
    // Check if we have code or file parameter
    FString PythonCode;
    FString PythonFile;
    bool hasCode = Params->TryGetStringField(FStringView(TEXT("code")), PythonCode);
    bool hasFile = Params->TryGetStringField(FStringView(TEXT("file")), PythonFile);

    // If code/file not found directly, check if they're in a 'data' object
    if (!hasCode && !hasFile)
    {
        const TSharedPtr<FJsonObject> *DataObject;
        if (Params->TryGetObjectField(FStringView(TEXT("data")), DataObject))
        {
            hasCode = (*DataObject)->TryGetStringField(FStringView(TEXT("code")), PythonCode);
            hasFile = (*DataObject)->TryGetStringField(FStringView(TEXT("file")), PythonFile);
        }
    }

    if (!hasCode && !hasFile)
    {
        MCP_LOG_WARNING("Missing 'code' or 'file' field in execute_python command");
        return CreateErrorResponse("Missing 'code' or 'file' field. You must provide either Python code or a file path.");
    }

    FString Result;
    bool bSuccess = false;
    FString ErrorMessage;

    if (hasCode)
    {
        // For code execution, we'll create a temporary file and execute that
        MCP_LOG_INFO("Executing Python code via temporary file");
        
        // Create a temporary file in the project's Saved/Temp directory
        FString TempDir = FPaths::ProjectSavedDir() / MCPConstants::PYTHON_TEMP_DIR_NAME;
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        
        // Ensure the directory exists
        if (!PlatformFile.DirectoryExists(*TempDir))
        {
            PlatformFile.CreateDirectory(*TempDir);
        }
        
        // Create a unique filename for the temporary Python script
        FString TempFilePath = TempDir / FString::Printf(TEXT("%s%s.py"), MCPConstants::PYTHON_TEMP_FILE_PREFIX, *FGuid::NewGuid().ToString());
        
        // Add error handling wrapper to the Python code
        FString WrappedPythonCode = TEXT("import sys\n")
            TEXT("import traceback\n")
            TEXT("import unreal\n\n")
            TEXT("# Create output capture file\n")
            TEXT("output_file = open('") + TempDir + TEXT("/output.txt', 'w')\n")
            TEXT("error_file = open('") + TempDir + TEXT("/error.txt', 'w')\n\n")
            TEXT("# Store original stdout and stderr\n")
            TEXT("original_stdout = sys.stdout\n")
            TEXT("original_stderr = sys.stderr\n\n")
            TEXT("# Redirect stdout and stderr\n")
            TEXT("sys.stdout = output_file\n")
            TEXT("sys.stderr = error_file\n\n")
            TEXT("success = True\n")
            TEXT("try:\n")
            // Instead of directly embedding the code, we'll compile it first to catch syntax errors
            TEXT("    # Compile the code to catch syntax errors\n")
            TEXT("    user_code = '''") + PythonCode + TEXT("'''\n")
            TEXT("    try:\n")
            TEXT("        code_obj = compile(user_code, '<string>', 'exec')\n")
            TEXT("        # Execute the compiled code\n")
            TEXT("        exec(code_obj)\n")
            TEXT("    except SyntaxError as e:\n")
            TEXT("        traceback.print_exc()\n")
            TEXT("        success = False\n")
            TEXT("    except Exception as e:\n")
            TEXT("        traceback.print_exc()\n")
            TEXT("        success = False\n")
            TEXT("except Exception as e:\n")
            TEXT("    traceback.print_exc()\n")
            TEXT("    success = False\n")
            TEXT("finally:\n")
            TEXT("    # Restore original stdout and stderr\n")
            TEXT("    sys.stdout = original_stdout\n")
            TEXT("    sys.stderr = original_stderr\n")
            TEXT("    output_file.close()\n")
            TEXT("    error_file.close()\n")
            TEXT("    # Write success status\n")
            TEXT("    with open('") + TempDir + TEXT("/status.txt', 'w') as f:\n")
            TEXT("        f.write('1' if success else '0')\n");
        
        // Write the Python code to the temporary file
        if (FFileHelper::SaveStringToFile(WrappedPythonCode, *TempFilePath))
        {
            // Execute the temporary file
            FString Command = FString::Printf(TEXT("py \"%s\""), *TempFilePath);
            GEngine->Exec(nullptr, *Command);
            
            // Read the output, error, and status files
            FString OutputContent;
            FString ErrorContent;
            FString StatusContent;
            
            FFileHelper::LoadFileToString(OutputContent, *(TempDir / TEXT("output.txt")));
            FFileHelper::LoadFileToString(ErrorContent, *(TempDir / TEXT("error.txt")));
            FFileHelper::LoadFileToString(StatusContent, *(TempDir / TEXT("status.txt")));
            
            bSuccess = StatusContent.TrimStartAndEnd().Equals(TEXT("1"));
            
            // Combine output and error for the result
            Result = OutputContent;
            ErrorMessage = ErrorContent;
            
            // Clean up the temporary files
            PlatformFile.DeleteFile(*TempFilePath);
            PlatformFile.DeleteFile(*(TempDir / TEXT("output.txt")));
            PlatformFile.DeleteFile(*(TempDir / TEXT("error.txt")));
            PlatformFile.DeleteFile(*(TempDir / TEXT("status.txt")));
        }
        else
        {
            MCP_LOG_ERROR("Failed to create temporary Python file at %s", *TempFilePath);
            return CreateErrorResponse(FString::Printf(TEXT("Failed to create temporary Python file at %s"), *TempFilePath));
        }
    }
    else if (hasFile)
    {
        // Execute Python file
        MCP_LOG_INFO("Executing Python file: %s", *PythonFile);
        
        // Create a temporary directory for output capture
        FString TempDir = FPaths::ProjectSavedDir() / MCPConstants::PYTHON_TEMP_DIR_NAME;
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        
        // Ensure the directory exists
        if (!PlatformFile.DirectoryExists(*TempDir))
        {
            PlatformFile.CreateDirectory(*TempDir);
        }
        
        // Create a wrapper script that executes the file and captures output
        FString WrapperFilePath = TempDir / FString::Printf(TEXT("%s_wrapper_%s.py"), MCPConstants::PYTHON_TEMP_FILE_PREFIX, *FGuid::NewGuid().ToString());
        
        FString WrapperCode = TEXT("import sys\n")
            TEXT("import traceback\n")
            TEXT("import unreal\n\n")
            TEXT("# Create output capture file\n")
            TEXT("output_file = open('") + TempDir + TEXT("/output.txt', 'w')\n")
            TEXT("error_file = open('") + TempDir + TEXT("/error.txt', 'w')\n\n")
            TEXT("# Store original stdout and stderr\n")
            TEXT("original_stdout = sys.stdout\n")
            TEXT("original_stderr = sys.stderr\n\n")
            TEXT("# Redirect stdout and stderr\n")
            TEXT("sys.stdout = output_file\n")
            TEXT("sys.stderr = error_file\n\n")
            TEXT("success = True\n")
            TEXT("try:\n")
            TEXT("    # Read the file content\n")
            TEXT("    with open('") + PythonFile.Replace(TEXT("\\"), TEXT("\\\\")) + TEXT("', 'r') as f:\n")
            TEXT("        file_content = f.read()\n")
            TEXT("    # Compile the code to catch syntax errors\n")
            TEXT("    try:\n")
            TEXT("        code_obj = compile(file_content, '") + PythonFile.Replace(TEXT("\\"), TEXT("\\\\")) + TEXT("', 'exec')\n")
            TEXT("        # Execute the compiled code\n")
            TEXT("        exec(code_obj)\n")
            TEXT("    except SyntaxError as e:\n")
            TEXT("        traceback.print_exc()\n")
            TEXT("        success = False\n")
            TEXT("    except Exception as e:\n")
            TEXT("        traceback.print_exc()\n")
            TEXT("        success = False\n")
            TEXT("except Exception as e:\n")
            TEXT("    traceback.print_exc()\n")
            TEXT("    success = False\n")
            TEXT("finally:\n")
            TEXT("    # Restore original stdout and stderr\n")
            TEXT("    sys.stdout = original_stdout\n")
            TEXT("    sys.stderr = original_stderr\n")
            TEXT("    output_file.close()\n")
            TEXT("    error_file.close()\n")
            TEXT("    # Write success status\n")
            TEXT("    with open('") + TempDir + TEXT("/status.txt', 'w') as f:\n")
            TEXT("        f.write('1' if success else '0')\n");
        
        if (FFileHelper::SaveStringToFile(WrapperCode, *WrapperFilePath))
        {
            // Execute the wrapper script
            FString Command = FString::Printf(TEXT("py \"%s\""), *WrapperFilePath);
            GEngine->Exec(nullptr, *Command);
            
            // Read the output, error, and status files
            FString OutputContent;
            FString ErrorContent;
            FString StatusContent;
            
            FFileHelper::LoadFileToString(OutputContent, *(TempDir / TEXT("output.txt")));
            FFileHelper::LoadFileToString(ErrorContent, *(TempDir / TEXT("error.txt")));
            FFileHelper::LoadFileToString(StatusContent, *(TempDir / TEXT("status.txt")));
            
            bSuccess = StatusContent.TrimStartAndEnd().Equals(TEXT("1"));
            
            // Combine output and error for the result
            Result = OutputContent;
            ErrorMessage = ErrorContent;
            
            // Clean up the temporary files
            PlatformFile.DeleteFile(*WrapperFilePath);
            PlatformFile.DeleteFile(*(TempDir / TEXT("output.txt")));
            PlatformFile.DeleteFile(*(TempDir / TEXT("error.txt")));
            PlatformFile.DeleteFile(*(TempDir / TEXT("status.txt")));
        }
        else
        {
            MCP_LOG_ERROR("Failed to create wrapper Python file at %s", *WrapperFilePath);
            return CreateErrorResponse(FString::Printf(TEXT("Failed to create wrapper Python file at %s"), *WrapperFilePath));
        }
    }

    // Create the response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField("output", Result);
    
    if (bSuccess)
    {
        MCP_LOG_INFO("Python execution successful");
        return CreateSuccessResponse(ResultObj);
    }
    else
    {
        MCP_LOG_ERROR("Python execution failed: %s", *ErrorMessage);
        ResultObj->SetStringField("error", ErrorMessage);
        
        // We're returning a success response with error details rather than an error response
        // This allows the client to still access the output and error information
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField("status", "error");
        Response->SetStringField("message", "Python execution failed with errors");
        Response->SetObjectField("result", ResultObj);
        return Response;
    }
}
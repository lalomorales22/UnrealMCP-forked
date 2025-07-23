#include "MCPConstants.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Interfaces/IPluginManager.h"

// Initialize the static path variables
FString MCPConstants::ProjectRootPath;
FString MCPConstants::PluginRootPath;
FString MCPConstants::PluginContentPath;
FString MCPConstants::PluginResourcesPath;
FString MCPConstants::PluginLogsPath;
FString MCPConstants::PluginMCPScriptsPath;

void MCPConstants::InitializePathConstants()
{
    // Get the project root path
    ProjectRootPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    
    // Get the plugin root path
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("UnrealArchitect");
    if (Plugin.IsValid())
    {
        PluginRootPath = FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir());
        
        // Derive other paths from the plugin root
        PluginContentPath = FPaths::Combine(PluginRootPath, TEXT("Content"));
        PluginResourcesPath = FPaths::Combine(PluginRootPath, TEXT("Resources"));
        PluginLogsPath = FPaths::Combine(PluginRootPath, TEXT("Logs"));
        PluginMCPScriptsPath = FPaths::Combine(PluginRootPath, TEXT("MCP"));
        
        // Ensure directories exist
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        
        if (!PlatformFile.DirectoryExists(*PluginLogsPath))
        {
            PlatformFile.CreateDirectory(*PluginLogsPath);
        }
        
        if (!PlatformFile.DirectoryExists(*PluginMCPScriptsPath))
        {
            PlatformFile.CreateDirectory(*PluginMCPScriptsPath);
        }
    }
    else
    {
        // Fallback to project-relative paths if plugin is not found
        PluginRootPath = FPaths::Combine(ProjectRootPath, TEXT("Plugins/UnrealArchitect"));
        PluginContentPath = FPaths::Combine(PluginRootPath, TEXT("Content"));
        PluginResourcesPath = FPaths::Combine(PluginRootPath, TEXT("Resources"));
        PluginLogsPath = FPaths::Combine(PluginRootPath, TEXT("Logs"));
        PluginMCPScriptsPath = FPaths::Combine(PluginRootPath, TEXT("MCP"));
    }
} 
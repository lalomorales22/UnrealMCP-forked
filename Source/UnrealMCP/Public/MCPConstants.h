#pragma once

#include "CoreMinimal.h"

/**
 * Constants used throughout the MCP plugin
 */
namespace MCPConstants
{
    // Network constants
    constexpr int32 DEFAULT_PORT = 13377;
    constexpr int32 DEFAULT_RECEIVE_BUFFER_SIZE = 32768; // 32KB buffer size
    constexpr int32 DEFAULT_SEND_BUFFER_SIZE = 32768;    // 32KB buffer size
    constexpr float DEFAULT_CLIENT_TIMEOUT_SECONDS = 30.0f;
    constexpr float DEFAULT_TICK_INTERVAL_SECONDS = 0.1f;
    
    // Python constants
    constexpr const TCHAR* PYTHON_TEMP_DIR_NAME = TEXT("PythonTemp");
    constexpr const TCHAR* PYTHON_TEMP_FILE_PREFIX = TEXT("mcp_temp_script_");
    
    // Logging constants
    constexpr bool DEFAULT_VERBOSE_LOGGING = false;
    
    // Performance constants
    constexpr int32 MAX_ACTORS_IN_SCENE_INFO = 100;
    
    // Path constants - use these instead of hardcoded paths
    // These will be initialized at runtime in the module startup
    extern FString ProjectRootPath;         // Root path of the project
    extern FString PluginRootPath;          // Root path of the MCP plugin
    extern FString PluginContentPath;       // Path to the plugin's content directory
    extern FString PluginResourcesPath;     // Path to the plugin's resources directory
    extern FString PluginLogsPath;          // Path to the plugin's logs directory
    extern FString PluginMCPScriptsPath;    // Path to the plugin's MCP scripts directory
    
    // Function to initialize all path variables at runtime
    void InitializePathConstants();
} 
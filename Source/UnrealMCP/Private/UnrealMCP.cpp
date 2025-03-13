// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealMCP.h"
#include "MCPTCPServer.h"
#include "MCPSettings.h"
#include "MCPConstants.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleMacros.h"
#include "ISettingsModule.h"
#include "ToolMenus.h"
#include "ToolMenuSection.h"
#include "MCPFileLogger.h"

// Define the log category
DEFINE_LOG_CATEGORY(LogMCP);

// Shorthand for logger
#define MCP_LOG(Verbosity, Format, ...) FMCPFileLogger::Get().Log(ELogVerbosity::Verbosity, FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_INFO(Format, ...) FMCPFileLogger::Get().Info(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_ERROR(Format, ...) FMCPFileLogger::Get().Error(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_WARNING(Format, ...) FMCPFileLogger::Get().Warning(FString::Printf(TEXT(Format), ##__VA_ARGS__))
#define MCP_LOG_VERBOSE(Format, ...) FMCPFileLogger::Get().Verbose(FString::Printf(TEXT(Format), ##__VA_ARGS__))

#define LOCTEXT_NAMESPACE "FUnrealMCPModule"

// Define a style set for our plugin
class FMCPPluginStyle : public FSlateStyleSet
{
public:
	FMCPPluginStyle() : FSlateStyleSet("MCPPluginStyle")
	{
		const FVector2D Icon16x16(16.0f, 16.0f);
		const FVector2D Icon20x20(20.0f, 20.0f);
		const FVector2D Icon40x40(40.0f, 40.0f);

		// Use path constants instead of finding the plugin each time
		SetContentRoot(MCPConstants::PluginResourcesPath);

		// Register icon
		FSlateImageBrush* MCPIconBrush = new FSlateImageBrush(RootToContentDir(TEXT("Icon128.png")), Icon16x16);
		Set("MCPPlugin.ServerIcon", MCPIconBrush);
	}

	static TSharedRef<FMCPPluginStyle> Create()
	{
		TSharedRef<FMCPPluginStyle> StyleRef = MakeShareable(new FMCPPluginStyle());
		return StyleRef;
	}
	
	static void Initialize()
	{
		if (!Instance.IsValid())
		{
			Instance = Create();
		}
	}

	static void Shutdown()
	{
		if (Instance.IsValid())
		{
			FSlateStyleRegistry::UnRegisterSlateStyle(*Instance);
			Instance.Reset();
		}
	}

	static TSharedPtr<FMCPPluginStyle> Get()
	{
		return Instance;
	}

private:
	static TSharedPtr<FMCPPluginStyle> Instance;
};

TSharedPtr<FMCPPluginStyle> FMCPPluginStyle::Instance = nullptr;

void FUnrealMCPModule::StartupModule()
{
	// Initialize path constants first
	MCPConstants::InitializePathConstants();
	
	// Initialize our custom log category
	MCP_LOG_WARNING("UnrealMCP Plugin is starting up");
	
	// Initialize file logger - now using path constants
	FString LogFilePath = FPaths::Combine(MCPConstants::PluginLogsPath, TEXT("MCPServer.log"));
	FMCPFileLogger::Get().Initialize(LogFilePath);
	
	// Register style set
	FMCPPluginStyle::Initialize();
	FSlateStyleRegistry::RegisterSlateStyle(*FMCPPluginStyle::Get());
	
	// More debug logging
	MCP_LOG_WARNING("UnrealMCP Style registered");

	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Editor", "Plugins", "MCP Settings",
			LOCTEXT("MCPSettingsName", "MCP Settings"),
			LOCTEXT("MCPSettingsDescription", "Configure the MCP plugin settings"),
			GetMutableDefault<UMCPSettings>()
		);
	}

	// Register for post engine init to add toolbar button
	// First, make sure we're not already registered
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	
	MCP_LOG_WARNING("Registering OnPostEngineInit delegate");
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FUnrealMCPModule::ExtendLevelEditorToolbar);
	
}

void FUnrealMCPModule::ShutdownModule()
{
	// Unregister style set
	FMCPPluginStyle::Shutdown();

	// Unregister settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Editor", "Plugins", "MCP Settings");
	}

	// Stop server if running
	if (Server)
	{
		StopServer();
	}
	
	// Clean up delegates
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
}

void FUnrealMCPModule::ExtendLevelEditorToolbar()
{
	static bool bToolbarExtended = false;
	
	if (bToolbarExtended)
	{
		MCP_LOG_WARNING("ExtendLevelEditorToolbar called but toolbar already extended, skipping");
		return;
	}
	
	MCP_LOG_WARNING("ExtendLevelEditorToolbar called - first time");
	
	// Register the main menu
	UToolMenus::Get()->RegisterMenu("LevelEditor.MainMenu", "MainFrame.MainMenu");
	
	// Add to the main toolbar
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
	if (ToolbarMenu)
	{
		FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("MCP");
		
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(
			"MCPServerToggle",
			FUIAction(
				FExecuteAction::CreateRaw(this, &FUnrealMCPModule::ToggleServer),
				FCanExecuteAction(),
				FIsActionChecked::CreateRaw(this, &FUnrealMCPModule::IsServerRunning)
			),
			LOCTEXT("MCPButtonLabel", "MCP Server"),
			LOCTEXT("MCPButtonTooltip", "Start/Stop MCP Server"),
			FSlateIcon(FMCPPluginStyle::Get()->GetStyleSetName(), "MCPPlugin.ServerIcon")
		));
		
		MCP_LOG_WARNING("MCP Server button added to main toolbar");
	}
	
	// Add to Window menu
	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	if (WindowMenu)
	{
		FToolMenuSection& Section = WindowMenu->FindOrAddSection("WindowLayout");
		Section.AddMenuEntry(
			"MCPServerToggleWindow",
			LOCTEXT("MCPWindowMenuLabel", "MCP Server"),
			LOCTEXT("MCPWindowMenuTooltip", "Start/Stop MCP Server"),
			FSlateIcon(FMCPPluginStyle::Get()->GetStyleSetName(), "MCPPlugin.ServerIcon"),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FUnrealMCPModule::ToggleServer),
				FCanExecuteAction(),
				FIsActionChecked::CreateRaw(this, &FUnrealMCPModule::IsServerRunning)
			),
			EUserInterfaceActionType::ToggleButton
		);
		MCP_LOG_WARNING("MCP Server entry added to Window menu");
	}
	
	bToolbarExtended = true;
}

// Legacy toolbar extension method - no longer used
void FUnrealMCPModule::AddToolbarButton(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnrealMCPModule::ToggleServer),
			FCanExecuteAction(),
			FIsActionChecked::CreateRaw(this, &FUnrealMCPModule::IsServerRunning)
		),
		NAME_None,
		LOCTEXT("MCPButtonLabel", "MCP Server"),
		LOCTEXT("MCPButtonTooltip", "Start/Stop MCP Server"),
		FSlateIcon(FMCPPluginStyle::Get()->GetStyleSetName(), "MCPPlugin.ServerIcon"),
		EUserInterfaceActionType::ToggleButton
	);
}

void FUnrealMCPModule::ToggleServer()
{
	MCP_LOG_WARNING("ToggleServer called - Server state: %s", (Server && Server->IsRunning()) ? TEXT("Running") : TEXT("Not Running"));
	
	if (Server && Server->IsRunning())
	{
		MCP_LOG_WARNING("Stopping server...");
		StopServer();
	}
	else
	{
		MCP_LOG_WARNING("Starting server...");
		StartServer();
	}
	
	MCP_LOG_WARNING("ToggleServer completed - Server state: %s", (Server && Server->IsRunning()) ? TEXT("Running") : TEXT("Not Running"));
}

void FUnrealMCPModule::StartServer()
{
	// Check if server is already running to prevent double-start
	if (Server && Server->IsRunning())
	{
		MCP_LOG_WARNING("Server is already running, ignoring start request");
		return;
	}

	MCP_LOG_WARNING("Creating new server instance");
	const UMCPSettings* Settings = GetDefault<UMCPSettings>();
	
	// Create a config object and set the port from settings
	FMCPTCPServerConfig Config;
	Config.Port = Settings->Port;
	
	// Create the server with the config
	Server = MakeUnique<FMCPTCPServer>(Config);
	
	if (Server->Start())
	{
		// The server already logs this message, so we don't need to log it here
		// MCP_LOG_INFO("MCP Server started on port %d", Settings->Port);
	}
	else
	{
		MCP_LOG_ERROR("Failed to start MCP Server");
	}
}

void FUnrealMCPModule::StopServer()
{
	if (Server)
	{
		Server->Stop();
		Server.Reset();
		MCP_LOG_INFO("MCP Server stopped");
	}
}

bool FUnrealMCPModule::IsServerRunning() const
{
	return Server && Server->IsRunning();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealMCPModule, UnrealMCP)
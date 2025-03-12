// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealMCP.h"
#include "MCPTCPServer.h"
#include "MCPSettings.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleMacros.h"
#include "ISettingsModule.h"
#include "ToolMenus.h"
#include "ToolMenuSection.h"

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

		SetContentRoot(IPluginManager::Get().FindPlugin("UnrealMCP")->GetBaseDir() / TEXT("Resources"));

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
	UE_LOG(LogTemp, Warning, TEXT("UnrealMCP Plugin is starting up"));
	
	// Register style set
	FMCPPluginStyle::Initialize();
	FSlateStyleRegistry::RegisterSlateStyle(*FMCPPluginStyle::Get());
	
	// More debug logging
	UE_LOG(LogTemp, Warning, TEXT("UnrealMCP Style registered"));

	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Editor", "Plugins", "MCP Settings",
			LOCTEXT("MCPSettingsName", "MCP Settings"),
			LOCTEXT("MCPSettingsDescription", "Configure the MCP plugin settings"),
			GetMutableDefault<UMCPSettings>()
		);
	}

	// Make sure UToolMenus is initialized
	/*
	if (!UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::Initialize();
		UE_LOG(LogTemp, Warning, TEXT("Initialized UToolMenus"));
	}
	*/

	// Register for post engine init to add toolbar button
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FUnrealMCPModule::ExtendLevelEditorToolbar);
	
	// Also try the legacy approach as a fallback
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension(
		"Settings",
		EExtensionHook::After,
		nullptr,
		FToolBarExtensionDelegate::CreateRaw(this, &FUnrealMCPModule::AddToolbarButton)
	);
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	UE_LOG(LogTemp, Warning, TEXT("Added legacy toolbar extender"));
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
		
		UE_LOG(LogTemp, Warning, TEXT("MCP Server button added to main toolbar"));
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
		UE_LOG(LogTemp, Warning, TEXT("MCP Server entry added to Window menu"));
	}
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
	if (Server && Server->IsRunning())
	{
		StopServer();
	}
	else
	{
		StartServer();
	}
}

void FUnrealMCPModule::StartServer()
{
	const UMCPSettings* Settings = GetDefault<UMCPSettings>();
	Server = MakeUnique<FMCPTCPServer>(Settings->Port);
	if (Server->Start())
	{
		UE_LOG(LogTemp, Log, TEXT("MCP Server started on port %d"), Settings->Port);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to start MCP Server"));
	}
}

void FUnrealMCPModule::StopServer()
{
	if (Server)
	{
		Server->Stop();
		Server.Reset();
		UE_LOG(LogTemp, Log, TEXT("MCP Server stopped"));
	}
}

bool FUnrealMCPModule::IsServerRunning() const
{
	return Server && Server->IsRunning();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealMCPModule, UnrealMCP)
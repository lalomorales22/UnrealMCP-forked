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
	// Register style set
	FMCPPluginStyle::Initialize();
	FSlateStyleRegistry::RegisterSlateStyle(*FMCPPluginStyle::Get());

	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Editor", "Plugins", "MCP Settings",
			LOCTEXT("MCPSettingsName", "MCP Settings"),
			LOCTEXT("MCPSettingsDescription", "Configure the MCP plugin settings"),
			GetMutableDefault<UMCPSettings>()
		);
	}

	// Add toolbar button
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension(
		"Settings",
		EExtensionHook::After,
		nullptr,
		FToolBarExtensionDelegate::CreateRaw(this, &FUnrealMCPModule::AddToolbarButton)
	);
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
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
}

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
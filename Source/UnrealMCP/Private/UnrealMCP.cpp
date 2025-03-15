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
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "EditorStyleSet.h"

// Define the log category
DEFINE_LOG_CATEGORY(LogMCP);

#define LOCTEXT_NAMESPACE "FUnrealMCPModule"

// Define a style set for our plugin
class FMCPPluginStyle : public FSlateStyleSet
{
public:
	FMCPPluginStyle() : FSlateStyleSet("MCPPluginStyle")
	{
		const FVector2D Icon16x16(16.0f, 16.0f);
		const FVector2D StatusSize(6.0f, 6.0f);

		// Use path constants instead of finding the plugin each time
		SetContentRoot(MCPConstants::PluginResourcesPath);

		// Register icon
		FSlateImageBrush* MCPIconBrush = new FSlateImageBrush(
			RootToContentDir(TEXT("Icon128.png")), 
			Icon16x16,
			FLinearColor::White,  // Tint (white preserves original colors)
			ESlateBrushTileType::NoTile  // Ensure no tiling, just the image
		);
		Set("MCPPlugin.ServerIcon", MCPIconBrush);

		// Create status indicator brushes
		const FLinearColor RunningColor(0.0f, 0.8f, 0.0f);  // Green
		const FLinearColor StoppedColor(0.8f, 0.0f, 0.0f);  // Red
		
		Set("MCPPlugin.StatusRunning", new FSlateRoundedBoxBrush(RunningColor, 3.0f, FVector2f(StatusSize)));
		Set("MCPPlugin.StatusStopped", new FSlateRoundedBoxBrush(StoppedColor, 3.0f, FVector2f(StatusSize)));

		// Define a custom button style with hover feedback
		FButtonStyle ToolbarButtonStyle = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("LevelEditor.ToolBar.Button");
        
		// Normal state: fully transparent background
		ToolbarButtonStyle.SetNormal(FSlateColorBrush(FLinearColor(0, 0, 0, 0))); // Transparent
        
		// Hovered state: subtle overlay (e.g., light gray with low opacity)
		ToolbarButtonStyle.SetHovered(FSlateColorBrush(FLinearColor(0.2f, 0.2f, 0.2f, 0.3f))); // Semi-transparent gray
        
		// Pressed state: slightly darker overlay
		ToolbarButtonStyle.SetPressed(FSlateColorBrush(FLinearColor(0.1f, 0.1f, 0.1f, 0.5f))); // Darker semi-transparent gray
        
		// Register the custom style
		Set("MCPPlugin.TransparentToolbarButton", ToolbarButtonStyle);
	}

	static void Initialize()
	{
		if (!Instance.IsValid())
		{
			Instance = MakeShareable(new FMCPPluginStyle());
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
	MCP_LOG_INFO("UnrealMCP Plugin is starting up");
	
	// Initialize file logger - now using path constants
	FString LogFilePath = FPaths::Combine(MCPConstants::PluginLogsPath, TEXT("MCPServer.log"));
	FMCPFileLogger::Get().Initialize(LogFilePath);
	
	// Register style set
	FMCPPluginStyle::Initialize();
	FSlateStyleRegistry::RegisterSlateStyle(*FMCPPluginStyle::Get());
	
	// More debug logging
	MCP_LOG_INFO("UnrealMCP Style registered");

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
	
	MCP_LOG_INFO("Registering OnPostEngineInit delegate");
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
	
	// Close control panel if open
	CloseMCPControlPanel();
	
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
    
    MCP_LOG_INFO("ExtendLevelEditorToolbar called - first time");
    
    UToolMenus::Get()->RegisterMenu("LevelEditor.MainMenu", "MainFrame.MainMenu");
    
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
    if (ToolbarMenu)
    {
        FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("MCP");
        
        // Add a custom widget instead of a static toolbar button
        Section.AddEntry(FToolMenuEntry::InitWidget(
            "MCPServerControl",
            SNew(SButton)
            .ButtonStyle(FMCPPluginStyle::Get().ToSharedRef(), "MCPPlugin.TransparentToolbarButton")
            //.ButtonStyle(FAppStyle::Get(), "LevelEditor.ToolBar.Button") // Match toolbar style
            .OnClicked(FOnClicked::CreateRaw(this, &FUnrealMCPModule::OpenMCPControlPanel_OnClicked))
            .ToolTipText(LOCTEXT("MCPButtonTooltip", "Open MCP Server Control Panel"))
            .Content()
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                    SNew(SImage)
                    .Image(FMCPPluginStyle::Get()->GetBrush("MCPPlugin.ServerIcon"))
                	.ColorAndOpacity(FLinearColor::White)  // Ensure no tint overrides transparency
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Right)
                .VAlign(VAlign_Bottom)
                [
                    SNew(SImage)
                    .Image_Lambda([this]() -> const FSlateBrush* {
                        return IsServerRunning() 
                            ? FMCPPluginStyle::Get()->GetBrush("MCPPlugin.StatusRunning") 
                            : FMCPPluginStyle::Get()->GetBrush("MCPPlugin.StatusStopped");
                    })
                ]
            ],
            FText::GetEmpty(),  // No label needed since the icon is visual
            true,   // bNoIndent
            false,  // bSearchable
            false
        ));
        
        MCP_LOG_INFO("MCP Server button added to main toolbar with dynamic icon");
    }
    
    // Window menu code remains unchanged
    UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    if (WindowMenu)
    {
        FToolMenuSection& Section = WindowMenu->FindOrAddSection("WindowLayout");
        Section.AddMenuEntry(
            "MCPServerControlWindow",
            LOCTEXT("MCPWindowMenuLabel", "MCP Server Control Panel"),
            LOCTEXT("MCPWindowMenuTooltip", "Open MCP Server Control Panel"),
            FSlateIcon(FMCPPluginStyle::Get()->GetStyleSetName(), "MCPPlugin.ServerIcon"),
            FUIAction(
                FExecuteAction::CreateRaw(this, &FUnrealMCPModule::OpenMCPControlPanel),
                FCanExecuteAction()
            )
        );
        MCP_LOG_INFO("MCP Server entry added to Window menu");
    }
    
    bToolbarExtended = true;
}

// Legacy toolbar extension method - no longer used
void FUnrealMCPModule::AddToolbarButton(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateRaw(this, &FUnrealMCPModule::OpenMCPControlPanel),
			FCanExecuteAction()
		),
		NAME_None,
		LOCTEXT("MCPButtonLabel", "MCP Server"),
		LOCTEXT("MCPButtonTooltip", "Open MCP Server Control Panel"),
		FSlateIcon(FMCPPluginStyle::Get()->GetStyleSetName(), "MCPPlugin.ServerIcon")
	);
}

void FUnrealMCPModule::OpenMCPControlPanel()
{
	// If the window already exists, just focus it
	if (MCPControlPanelWindow.IsValid())
	{
		MCPControlPanelWindow->BringToFront();
		return;
	}

	// Create a new window
	MCPControlPanelWindow = SNew(SWindow)
		.Title(LOCTEXT("MCPControlPanelTitle", "MCP Server Control Panel"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.HasCloseButton(true)
		.CreateTitleBar(true)
		.IsTopmostWindow(true)
		.MinWidth(300)
		.MinHeight(150);

	// Set the content of the window
	MCPControlPanelWindow->SetContent(CreateMCPControlPanelContent());

	// Register a callback for when the window is closed
	MCPControlPanelWindow->GetOnWindowClosedEvent().AddRaw(this, &FUnrealMCPModule::OnMCPControlPanelClosed);

	// Show the window
	FSlateApplication::Get().AddWindow(MCPControlPanelWindow.ToSharedRef());

	MCP_LOG_INFO("MCP Control Panel opened");
}

FReply FUnrealMCPModule::OpenMCPControlPanel_OnClicked()
{
	OpenMCPControlPanel();

	return FReply::Handled();
}

void FUnrealMCPModule::OnMCPControlPanelClosed(const TSharedRef<SWindow>& Window)
{
	MCPControlPanelWindow.Reset();
	MCP_LOG_INFO("MCP Control Panel closed");
}

void FUnrealMCPModule::CloseMCPControlPanel()
{
	if (MCPControlPanelWindow.IsValid())
	{
		MCPControlPanelWindow->RequestDestroyWindow();
		MCPControlPanelWindow.Reset();
		MCP_LOG_INFO("MCP Control Panel closed");
	}
}

TSharedRef<SWidget> FUnrealMCPModule::CreateMCPControlPanelContent()
{
	const UMCPSettings* Settings = GetDefault<UMCPSettings>();
	
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)
			
			// Status section
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SHorizontalBox)
				
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 8, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ServerStatusLabel", "Server Status:"))
					.Font(FAppStyle::GetFontStyle("NormalText"))
				]
				
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() -> FText {
						return IsServerRunning() 
							? LOCTEXT("ServerRunningStatus", "Running") 
							: LOCTEXT("ServerStoppedStatus", "Stopped");
					})
					.ColorAndOpacity_Lambda([this]() -> FSlateColor {
						return IsServerRunning() 
							? FSlateColor(FLinearColor(0.0f, 0.8f, 0.0f)) 
							: FSlateColor(FLinearColor(0.8f, 0.0f, 0.0f));
					})
					.Font(FAppStyle::GetFontStyle("NormalText"))
				]
			]
			
			// Port information
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SHorizontalBox)
				
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 8, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ServerPortLabel", "Port:"))
					.Font(FAppStyle::GetFontStyle("NormalText"))
				]
				
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::FromInt(Settings->Port)))
					.Font(FAppStyle::GetFontStyle("NormalText"))
				]
			]
			
			// Buttons
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 8, 0, 0)
			.HAlign(HAlign_Center)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(5.0f))
				.MinDesiredSlotWidth(100.0f)
				
				// Start button
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Text(LOCTEXT("StartServerButton", "Start Server"))
					.IsEnabled_Lambda([this]() -> bool { return !IsServerRunning(); })
					.OnClicked(FOnClicked::CreateRaw(this, &FUnrealMCPModule::OnStartServerClicked))
				]
				
				// Stop button
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Text(LOCTEXT("StopServerButton", "Stop Server"))
					.IsEnabled_Lambda([this]() -> bool { return IsServerRunning(); })
					.OnClicked(FOnClicked::CreateRaw(this, &FUnrealMCPModule::OnStopServerClicked))
				]
			]
			
			// Settings button
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 16, 0, 0)
			.HAlign(HAlign_Center)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("OpenSettingsButton", "Open Settings"))
				.OnClicked_Lambda([this]() -> FReply {
					if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
					{
						SettingsModule->ShowViewer("Editor", "Plugins", "MCP Settings");
					}
					return FReply::Handled();
				})
			]
		];
}

FReply FUnrealMCPModule::OnStartServerClicked()
{
	StartServer();
	return FReply::Handled();
}

FReply FUnrealMCPModule::OnStopServerClicked()
{
	StopServer();
	return FReply::Handled();
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
		// Refresh the toolbar to update the status indicator
		if (UToolMenus* ToolMenus = UToolMenus::Get())
		{
			ToolMenus->RefreshAllWidgets();
		}
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
		
		// Refresh the toolbar to update the status indicator
		if (UToolMenus* ToolMenus = UToolMenus::Get())
		{
			ToolMenus->RefreshAllWidgets();
		}
	}
}

bool FUnrealMCPModule::IsServerRunning() const
{
	return Server && Server->IsRunning();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealMCPModule, UnrealMCP)
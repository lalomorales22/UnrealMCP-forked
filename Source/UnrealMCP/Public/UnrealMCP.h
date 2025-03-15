// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Declare custom log category
UNREALMCP_API DECLARE_LOG_CATEGORY_EXTERN(LogMCP, Log, All);

class FMCPTCPServer;
class SWindow;

class FUnrealMCPModule : public IModuleInterface, public TSharedFromThis<FUnrealMCPModule>
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Get the MCP server instance
	 * External modules can use this to register custom handlers
	 * @return The MCP server instance, or nullptr if not available
	 */
	UNREALMCP_API FMCPTCPServer* GetServer() const { return Server.Get(); }

private:
	void ExtendLevelEditorToolbar();
	void AddToolbarButton(FToolBarBuilder& Builder);
	void ToggleServer();
	void StartServer();
	void StopServer();
	bool IsServerRunning() const;
	
	// MCP Control Panel functions
	void OpenMCPControlPanel();
	FReply OpenMCPControlPanel_OnClicked();
	void CloseMCPControlPanel();
	void OnMCPControlPanelClosed(const TSharedRef<SWindow>& Window);
	TSharedRef<class SWidget> CreateMCPControlPanelContent();
	FReply OnStartServerClicked();
	FReply OnStopServerClicked();
	
	TUniquePtr<FMCPTCPServer> Server;
	TSharedPtr<SWindow> MCPControlPanelWindow;
};

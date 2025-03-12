// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMCPTCPServer;

class FUnrealMCPModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void ExtendLevelEditorToolbar();
	void AddToolbarButton(FToolBarBuilder& Builder);
	void ToggleServer();
	void StartServer();
	void StopServer();
	bool IsServerRunning() const;

	TUniquePtr<FMCPTCPServer> Server;
};

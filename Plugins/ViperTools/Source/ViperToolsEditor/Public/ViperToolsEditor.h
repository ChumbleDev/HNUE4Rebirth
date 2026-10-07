// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FBlueprintEditor;
class FWorkflowAllowedTabSet;
class FSpawnTabArgs;
class SDockTab;

VIPERTOOLSEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogViperTools, Log, All);

class FViperToolsEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static const FName ParameterBehaviorsTabId;

private:
	/** Fired by the editor after any Blueprint finishes compiling (no way to know which one from this delegate alone, so we re-sync everything loaded). */
	void HandleBlueprintCompiled();

	/** Fired by the Kismet module whenever a Blueprint editor builds its tab set -- lets us add our own "Viper Tools" tab alongside the stock ones. */
	void HandleRegisterTabsForEditor(FWorkflowAllowedTabSet& TabFactories, FName ModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor);

	/** Spawns the global, Blueprint-editor-independent "Parameter Behaviors" tool window. */
	TSharedRef<SDockTab> SpawnParameterBehaviorsTab(const FSpawnTabArgs& Args);

	FDelegateHandle BlueprintCompiledHandle;
	FDelegateHandle RegisterTabsHandle;
};

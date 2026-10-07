// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsEditor.h"
#include "ViperToolsExpandEnumAsExecsSync.h"
#include "ViperToolsExecExpanderTabSummoner.h"
#include "ViperToolsDynamicOutputSync.h"
#include "ViperToolsDynamicOutputTabSummoner.h"
#include "ViperToolsDefaultToSelfSync.h"
#include "ViperToolsAdvancedDisplaySync.h"
#include "SViperToolsParameterBehaviorsPanel.h"
#include "Editor.h"
#include "BlueprintEditorModule.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructureModule.h"
#include "WorkspaceMenuStructure.h"

#define LOCTEXT_NAMESPACE "FViperToolsEditorModule"

DEFINE_LOG_CATEGORY(LogViperTools);

const FName FViperToolsEditorModule::ParameterBehaviorsTabId(TEXT("ViperToolsParameterBehaviors"));

void FViperToolsEditorModule::StartupModule()
{
	if (GEditor)
	{
		BlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddRaw(this, &FViperToolsEditorModule::HandleBlueprintCompiled);
	}

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	RegisterTabsHandle = BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FViperToolsEditorModule::HandleRegisterTabsForEditor);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ParameterBehaviorsTabId, FOnSpawnTab::CreateRaw(this, &FViperToolsEditorModule::SpawnParameterBehaviorsTab))
		.SetDisplayName(LOCTEXT("ParameterBehaviorsTabTitle", "Parameter Behaviors"))
		.SetTooltipText(LOCTEXT("ParameterBehaviorsTabTooltip", "Configure Expandable Enum, Dynamic Output, Default To Self, and Advanced/Hidden Pin behaviors for any Blueprint function (ViperTools)."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void FViperToolsEditorModule::ShutdownModule()
{
	if (GEditor && BlueprintCompiledHandle.IsValid())
	{
		GEditor->OnBlueprintCompiled().Remove(BlueprintCompiledHandle);
		BlueprintCompiledHandle.Reset();
	}

	if (FModuleManager::Get().IsModuleLoaded("Kismet") && RegisterTabsHandle.IsValid())
	{
		FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");
		BlueprintEditorModule.OnRegisterTabsForEditor().Remove(RegisterTabsHandle);
		RegisterTabsHandle.Reset();
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ParameterBehaviorsTabId);
}

void FViperToolsEditorModule::HandleBlueprintCompiled()
{
	FViperToolsExpandEnumAsExecsSync::SyncAllBlueprints();
	FViperToolsDynamicOutputSync::SyncAllBlueprints();
	FViperToolsDefaultToSelfSync::SyncAllBlueprints();
	FViperToolsAdvancedDisplaySync::SyncAllBlueprints();
}

void FViperToolsEditorModule::HandleRegisterTabsForEditor(FWorkflowAllowedTabSet& TabFactories, FName ModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor)
{
	TabFactories.RegisterFactory(MakeShareable(new FViperToolsExecExpanderTabSummoner(BlueprintEditor)));
	TabFactories.RegisterFactory(MakeShareable(new FViperToolsDynamicOutputTabSummoner(BlueprintEditor)));
}

TSharedRef<SDockTab> FViperToolsEditorModule::SpawnParameterBehaviorsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SViperToolsParameterBehaviorsPanel)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FViperToolsEditorModule, ViperToolsEditor)

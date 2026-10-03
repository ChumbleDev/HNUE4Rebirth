// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsEditor.h"
#include "ViperToolsExpandEnumAsExecsSync.h"
#include "ViperToolsExecExpanderTabSummoner.h"
#include "ViperToolsDynamicOutputSync.h"
#include "ViperToolsDynamicOutputTabSummoner.h"
#include "Editor.h"
#include "BlueprintEditorModule.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

#define LOCTEXT_NAMESPACE "FViperToolsEditorModule"

DEFINE_LOG_CATEGORY(LogViperTools);

void FViperToolsEditorModule::StartupModule()
{
	if (GEditor)
	{
		BlueprintCompiledHandle = GEditor->OnBlueprintCompiled().AddRaw(this, &FViperToolsEditorModule::HandleBlueprintCompiled);
	}

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	RegisterTabsHandle = BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FViperToolsEditorModule::HandleRegisterTabsForEditor);
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
}

void FViperToolsEditorModule::HandleBlueprintCompiled()
{
	FViperToolsExpandEnumAsExecsSync::SyncAllBlueprints();
	FViperToolsDynamicOutputSync::SyncAllBlueprints();
}

void FViperToolsEditorModule::HandleRegisterTabsForEditor(FWorkflowAllowedTabSet& TabFactories, FName ModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor)
{
	TabFactories.RegisterFactory(MakeShareable(new FViperToolsExecExpanderTabSummoner(BlueprintEditor)));
	TabFactories.RegisterFactory(MakeShareable(new FViperToolsDynamicOutputTabSummoner(BlueprintEditor)));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FViperToolsEditorModule, ViperToolsEditor)

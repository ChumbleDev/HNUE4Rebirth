// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsExecExpanderTabSummoner.h"
#include "SViperToolsExecExpanderPanel.h"
#include "BlueprintEditor.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FViperToolsExecExpanderTabSummoner"

const FName FViperToolsExecExpanderTabSummoner::TabId(TEXT("ViperToolsExecExpander"));

FViperToolsExecExpanderTabSummoner::FViperToolsExecExpanderTabSummoner(TSharedPtr<FBlueprintEditor> InBlueprintEditor)
	: FWorkflowTabFactory(TabId, InBlueprintEditor)
	, BlueprintEditorWeak(InBlueprintEditor)
{
	TabLabel = LOCTEXT("TabLabel", "Expandable Enums");
	TabIcon = FSlateIcon(FName("EditorStyle"), "BlueprintEditor.FindInBlueprint.MenuIcon");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("ViewMenuDescription", "Expandable Enums");
	ViewMenuTooltip = LOCTEXT("ViewMenuTooltip", "Flag enum parameters on your custom functions to expand them into exec pins (ViperTools).");
}

TSharedRef<SWidget> FViperToolsExecExpanderTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SViperToolsExecExpanderPanel, BlueprintEditorWeak);
}

#undef LOCTEXT_NAMESPACE

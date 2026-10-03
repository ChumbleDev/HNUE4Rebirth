// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsDynamicOutputTabSummoner.h"
#include "SViperToolsDynamicOutputPanel.h"
#include "BlueprintEditor.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FViperToolsDynamicOutputTabSummoner"

const FName FViperToolsDynamicOutputTabSummoner::TabId(TEXT("ViperToolsDynamicOutput"));

FViperToolsDynamicOutputTabSummoner::FViperToolsDynamicOutputTabSummoner(TSharedPtr<FBlueprintEditor> InBlueprintEditor)
	: FWorkflowTabFactory(TabId, InBlueprintEditor)
	, BlueprintEditorWeak(InBlueprintEditor)
{
	TabLabel = LOCTEXT("TabLabel", "Dynamic Output Types");
	TabIcon = FSlateIcon(FName("EditorStyle"), "BlueprintEditor.FindInBlueprint.MenuIcon");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("ViewMenuDescription", "Dynamic Output Types");
	ViewMenuTooltip = LOCTEXT("ViewMenuTooltip", "Make a function's output type follow an input Class parameter, just like native DeterminesOutputType (ViperTools).");
}

TSharedRef<SWidget> FViperToolsDynamicOutputTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SViperToolsDynamicOutputPanel, BlueprintEditorWeak);
}

#undef LOCTEXT_NAMESPACE

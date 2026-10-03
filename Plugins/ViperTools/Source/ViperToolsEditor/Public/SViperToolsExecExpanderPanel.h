// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"

class FBlueprintEditor;
class UEdGraph;
class SVerticalBox;

/**
 * Dedicated "Viper Tools" tab content for the Blueprint function editor: lists every enum-typed
 * input/output parameter across the Blueprint's custom functions, with a checkbox to flag it for
 * "expand as exec pins" behavior (see FViperToolsExpandEnumAsExecsSync for how that's applied).
 */
class SViperToolsExecExpanderPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SViperToolsExecExpanderPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FBlueprintEditor> InBlueprintEditor);

private:
	TWeakPtr<FBlueprintEditor> BlueprintEditorWeak;
	TSharedPtr<SVerticalBox> RowsContainer;

	FReply OnRefreshClicked();
	void RebuildRows();

	ECheckBoxState GetCheckState(TWeakObjectPtr<UEdGraph> Graph, FName ParamName) const;
	void OnCheckStateChanged(ECheckBoxState NewState, TWeakObjectPtr<UEdGraph> Graph, FName ParamName);
};

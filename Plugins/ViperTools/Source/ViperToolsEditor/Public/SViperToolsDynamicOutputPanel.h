// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"

class FBlueprintEditor;
class UEdGraph;
class SVerticalBox;

/**
 * "Dynamic Output Types" tab content for the Blueprint function editor: for each custom function,
 * lets the user pick one Class/Object input parameter as the "type picker" (equivalent to C++
 * meta=(DeterminesOutputType="Param")), and optionally flag specific Object/Class output parameters to
 * retype based on it (equivalent to meta=(DynamicOutputParam="Param1,Param2")). If no outputs are
 * flagged, the function's return value is retyped by default -- matching native engine behavior.
 */
class SViperToolsDynamicOutputPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SViperToolsDynamicOutputPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<FBlueprintEditor> InBlueprintEditor);

private:
	TWeakPtr<FBlueprintEditor> BlueprintEditorWeak;
	TSharedPtr<SVerticalBox> RowsContainer;

	FReply OnRefreshClicked();
	void RebuildRows();

	ECheckBoxState GetPickerCheckState(TWeakObjectPtr<UEdGraph> Graph, FName ParamName) const;
	void OnPickerCheckStateChanged(ECheckBoxState NewState, TWeakObjectPtr<UEdGraph> Graph, FName ParamName);

	ECheckBoxState GetOutputCheckState(TWeakObjectPtr<UEdGraph> Graph, FName ParamName) const;
	void OnOutputCheckStateChanged(ECheckBoxState NewState, TWeakObjectPtr<UEdGraph> Graph, FName ParamName);
};

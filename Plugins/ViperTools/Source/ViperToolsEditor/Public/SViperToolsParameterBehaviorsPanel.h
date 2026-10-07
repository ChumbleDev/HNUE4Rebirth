// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Input/Reply.h"

class UBlueprint;
class UEdGraph;
class UEdGraphPin;
class SVerticalBox;
class SEditableTextBox;

/** Which parameter-level behavior is currently being configured in the panel. */
enum class EViperParamBehaviorMode : uint8
{
	ExpandAsExecs,
	OutputTypePicker,
	DynamicOutputParam,
	DefaultToSelf,
	AdvancedDisplay
};

/**
 * Global "Parameter Behaviors" tool (NOT tied to any open Blueprint editor): pick a class, type the
 * exact (case-sensitive) name of one of its Blueprint functions, pick which behavior you want to
 * configure, then check the parameters that should get it. Unifies the Expandable Enums / Dynamic
 * Output Types tabs plus two new behaviors (Default To Self, Advanced/Hidden Pins) into one workflow,
 * without a free-text generic metadata editor -- each behavior keeps its own validated parameter
 * filtering, backed by the exact same per-feature metadata/sync classes the per-Blueprint-editor tabs
 * already use.
 *
 * Only supports Blueprint-generated classes and Blueprint *functions* -- macros have no underlying
 * UFunction for any of these behaviors to attach to (a macro call node clones the macro graph inline
 * at compile time instead of calling a real function), so they're not offered here.
 */
class SViperToolsParameterBehaviorsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SViperToolsParameterBehaviorsPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// --- Class + function selection ---
	TWeakObjectPtr<const UClass> SelectedClass;
	TSharedPtr<SEditableTextBox> FunctionNameBox;
	FString FunctionNameInput;

	TWeakObjectPtr<UBlueprint> ResolvedBlueprint;
	TWeakObjectPtr<UEdGraph> ResolvedGraph;
	FText StatusText;

	const UClass* GetSelectedClass() const;
	void OnClassPicked(const UClass* NewClass);

	void OnFunctionNameTextChanged(const FText& NewText);
	FReply OnFindFunctionClicked();
	void ResolveFunction();

	FText GetStatusText() const;

	// --- Behavior mode selection ---
	EViperParamBehaviorMode CurrentMode = EViperParamBehaviorMode::ExpandAsExecs;

	ECheckBoxState GetModeCheckState(EViperParamBehaviorMode Mode) const;
	void OnModeCheckStateChanged(ECheckBoxState NewState, EViperParamBehaviorMode Mode);
	static FText GetModeLabel(EViperParamBehaviorMode Mode);
	static FText GetModeTooltip(EViperParamBehaviorMode Mode);

	// --- Parameter rows for the current mode ---
	TSharedPtr<SVerticalBox> RowsContainer;

	void RebuildParameterRows();
	void GatherCandidatePins(EViperParamBehaviorMode Mode, TArray<UEdGraphPin*>& OutPins) const;

	ECheckBoxState GetParamCheckState(FName ParamName) const;
	void OnParamCheckStateChanged(ECheckBoxState NewState, FName ParamName);

	static bool IsEnumPin(const UEdGraphPin* Pin);
	static bool IsObjectOrClassLikePin(const UEdGraphPin* Pin);
	static bool IsSelfLikePin(const UEdGraphPin* Pin);
	static bool IsAdvancedDisplayCandidatePin(const UEdGraphPin* Pin);

	// --- Apply ---
	FReply OnApplyClicked();
};

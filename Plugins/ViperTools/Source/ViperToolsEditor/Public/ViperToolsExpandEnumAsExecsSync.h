// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Lets Blueprint-authored functions opt into the same "expand enum as exec pins" behavior that C++
 * UFUNCTIONs get via meta=(ExpandEnumAsExecs="ParamName") -- WITHOUT any engine source modification.
 *
 * Why this works: UK2Node_CallFunction decides whether to expand an enum parameter into exec pins by
 * checking UFunction::HasMetaData("ExpandEnumAsExecs") at the time a call node is created/refreshed.
 * That check is completely generic -- it doesn't care whether the UFunction came from a C++ UFUNCTION
 * or from a compiled Blueprint. Works for input enum parameters (on the function's FunctionEntry node)
 * and output/return enum parameters (on its FunctionResult node) alike.
 *
 * The user flags parameters via the dedicated "Viper Tools" tab in the Blueprint function editor
 * (see SViperToolsExecExpanderPanel), which stores the flags as UMetaData on the function's UEdGraph
 * (see FViperToolsExpandExecsMetadata) -- not by repurposing any existing UI field like Keywords.
 *
 * Since compiling a Blueprint regenerates its UFunctions from scratch, this class re-applies the real
 * engine metadata onto every freshly compiled UFunction, every time any Blueprint compiles.
 *
 * One engine quirk this class also works around: UK2Node_CallFunction names the generated exec pins
 * from UEnum::GetNameStringByIndex (the enum's internal entry name), never from its display name map.
 * For native C++ enums those are the same thing, but Blueprint-authored ("user defined") enums store
 * the user's friendly text separately (internal names look like "NewEnumerator0", "NewEnumerator1"...).
 * This is a hardcoded engine behavior, not something our ExpandEnumAsExecs metadata controls. We work
 * around it -- again without any engine source modification -- by setting UEdGraphPin::PinFriendlyName
 * on the generated exec pins to the enum's real display text; UEdGraphPin::GetDisplayName() already
 * prefers PinFriendlyName over the raw pin name when it's set, so the graph renders the friendly label
 * while the underlying pin name (used for wiring/compilation) is left untouched.
 */
class FViperToolsExpandEnumAsExecsSync
{
public:
	/** Re-scans every loaded Blueprint and reapplies ExpandEnumAsExecs metadata based on the flags stored via FViperToolsExpandExecsMetadata. Call this after any Blueprint compile. */
	static void SyncAllBlueprints();

private:
	static void ApplyToFunction(class UFunction* Function, const TArray<FName>& FlaggedParams);
	static void SyncBlueprint(class UBlueprint* Blueprint);

	/** Relabels exec pins generated from enum expansion (on every graph in Blueprint) to use the enum's friendly display text instead of its raw internal entry name. */
	static void FixUpEnumExecPinLabels(class UBlueprint* Blueprint);
	static void FixUpEnumExecPinLabelsInGraph(class UEdGraph* Graph);
	static void FixUpEnumExecPinLabelsOnNode(class UK2Node_CallFunction* CallFunctionNode);
};

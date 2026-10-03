// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Lets Blueprint-authored functions opt into the same "output type follows an input class" behavior
 * that C++ UFUNCTIONs get via meta=(DeterminesOutputType="ClassParam") -- the same mechanism behind
 * engine nodes like GetComponentByClass -- WITHOUT any engine source modification.
 *
 * Confirmed against engine source (K2Node_CallFunction.cpp, FDynamicOutputHelper): both
 * UFunction::GetMetaData("DeterminesOutputType") (which input picks the type) and
 * UFunction::GetMetaData("DynamicOutputParam") (which output(s) get retyped -- defaults to the return
 * value if left empty, exactly like native DeterminesOutputType functions) are read generically off the
 * live UFunction, regardless of whether it came from C++ or a compiled Blueprint.
 *
 * One engine quirk this class works around: the native "defaults to Return Value when left empty"
 * behavior (FDynamicOutputHelper::GetDynamicOutPins) only kicks in for a parameter that literally has
 * the CPF_ReturnParm flag -- and the Blueprint compiler (FKismetCompilerContext::FinishCompilingFunction)
 * only ever sets that flag on an output property literally named "ReturnValue". A Blueprint function
 * with a single custom-named output (e.g. "Extensions") never gets that flag, so the native default
 * silently retypes nothing. We work around this -- again without touching engine source -- by, when the
 * user hasn't explicitly flagged any dynamic output, inspecting the function's defining graph ourselves
 * and falling back to its sole Class/Object-like output parameter (the same candidates the "Dynamic
 * Outputs" checklist in SViperToolsDynamicOutputPanel already shows), then passing that along explicitly
 * via DynamicOutputParam metadata instead of relying on the engine's name-based default.
 *
 * The user configures this via the "Dynamic Output Types" tab in the Blueprint function editor (see
 * SViperToolsDynamicOutputPanel), which stores the configuration as UMetaData on the function's UEdGraph
 * (see FViperToolsDynamicOutputMetadata). This class reapplies that configuration onto the real,
 * freshly-compiled UFunction every time any Blueprint compiles.
 */
class FViperToolsDynamicOutputSync
{
public:
	/** Re-scans every loaded Blueprint and reapplies DeterminesOutputType/DynamicOutputParam metadata based on the configuration stored via FViperToolsDynamicOutputMetadata. Call this after any Blueprint compile. */
	static void SyncAllBlueprints();

private:
	static void ApplyToFunction(class UFunction* Function, FName PickerParam, const TArray<FName>& DynamicOutputParams);
	static void SyncBlueprint(class UBlueprint* Blueprint);

	/** If no dynamic outputs are explicitly flagged on Graph, returns Graph's sole Class/Object-like output parameter (if exactly one exists) as an implicit fallback; otherwise returns an empty array. */
	static TArray<FName> ComputeImplicitDynamicOutputParams(class UEdGraph* Graph);
};

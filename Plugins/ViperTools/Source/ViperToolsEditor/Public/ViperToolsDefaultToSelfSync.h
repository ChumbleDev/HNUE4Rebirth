// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Lets Blueprint-authored functions opt into the same "default this parameter to Self" behavior that
 * C++ UFUNCTIONs get via meta=(DefaultToSelf="ParamName") -- WITHOUT any engine source modification.
 *
 * Confirmed against engine source (K2Node_CallFunction.cpp): UFunction::GetMetaData("DefaultToSelf")
 * is read generically off the live UFunction at multiple points (AllocateDefaultPins, pin-hiding
 * refresh, and ExpandNode's static-function self-forwarding), regardless of whether the UFunction came
 * from C++ or a compiled Blueprint -- the exact same pattern already proven for DeterminesOutputType/
 * DynamicOutputParam/ExpandEnumAsExecs.
 *
 * The user configures this via the "Parameter Behaviors" tool (see SViperToolsParameterBehaviorsPanel),
 * which stores the flagged parameter as UMetaData on the function's UEdGraph (see
 * FViperToolsDefaultToSelfMetadata). This class reapplies that configuration onto the real,
 * freshly-compiled UFunction every time any Blueprint compiles.
 */
class FViperToolsDefaultToSelfSync
{
public:
	/** Re-scans every loaded Blueprint and reapplies DefaultToSelf metadata based on the configuration stored via FViperToolsDefaultToSelfMetadata. Call this after any Blueprint compile. */
	static void SyncAllBlueprints();

private:
	static void ApplyToFunction(class UFunction* Function, FName ParamName);
	static void SyncBlueprint(class UBlueprint* Blueprint);
};

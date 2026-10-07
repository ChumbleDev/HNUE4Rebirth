// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Lets Blueprint-authored functions opt into the same "advanced/hidden pin" behavior that C++
 * UFUNCTIONs get by marking a UPARAM AdvancedDisplay -- the Lerp-style collapsed-pin arrow -- WITHOUT
 * any engine source modification.
 *
 * Unlike DeterminesOutputType/DynamicOutputParam/ExpandEnumAsExecs/DefaultToSelf (all plain UMetaData
 * strings read off the UFunction), this one is a property FLAG: UK2Node_CallFunction::AllocateDefaultPins
 * checks Param->HasAllPropertyFlags(CPF_AdvancedDisplay) to decide whether a parameter's pin gets
 * Pin->bAdvancedView = true and collapses the node's AdvancedPinDisplay arrow -- completely generically,
 * regardless of whether the FProperty came from C++ UHT or Blueprint compilation. FProperty::
 * SetPropertyFlags/ClearPropertyFlags are public mutators, so we can set/clear this bit on the real
 * compiled property at sync time, same as we set UMetaData strings for the other features.
 *
 * The user configures this via the "Parameter Behaviors" tool (see SViperToolsParameterBehaviorsPanel),
 * which stores the flagged parameters as UMetaData on the function's UEdGraph (see
 * FViperToolsAdvancedDisplayMetadata) -- metadata is just the storage mechanism here; the actual
 * engine-facing effect is this property flag. This class reapplies that configuration onto the real,
 * freshly-compiled UFunction's properties every time any Blueprint compiles.
 */
class FViperToolsAdvancedDisplaySync
{
public:
	/** Re-scans every loaded Blueprint and reapplies CPF_AdvancedDisplay based on the configuration stored via FViperToolsAdvancedDisplayMetadata. Call this after any Blueprint compile. */
	static void SyncAllBlueprints();

private:
	static void ApplyToFunction(class UFunction* Function, const TArray<FName>& FlaggedParams);
	static void SyncBlueprint(class UBlueprint* Blueprint);
};

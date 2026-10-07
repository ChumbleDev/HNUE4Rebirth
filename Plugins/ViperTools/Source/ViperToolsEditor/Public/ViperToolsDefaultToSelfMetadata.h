// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Persistent storage for Blueprint-authored functions' "Default To Self" configuration -- mirrors
 * FViperToolsExpandExecsMetadata/FViperToolsDynamicOutputMetadata's approach (UMetaData on the
 * function's UEdGraph). Tracks at most one input Object/Interface parameter per function -- the
 * native equivalent of meta=(DefaultToSelf="ParamName"), which hides that pin and auto-wires it to
 * the calling graph's own Self reference when left unconnected.
 */
class FViperToolsDefaultToSelfMetadata
{
public:
	/** The input parameter flagged as "Default To Self" for this function's graph, or NAME_None if unset. */
	static FName GetDefaultToSelfParam(const class UEdGraph* Graph);

	/** Sets (or clears, passing NAME_None) the "Default To Self" parameter for this function's graph. Only one is allowed, so this replaces any previous one. */
	static void SetDefaultToSelfParam(class UEdGraph* Graph, FName ParamName);

private:
	static const FName MetaKey;
};

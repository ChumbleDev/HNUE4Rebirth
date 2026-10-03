// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Persistent storage for Blueprint-authored functions' "Determines Output Type" configuration --
 * mirrors FViperToolsExpandExecsMetadata's approach (UMetaData on the function's UEdGraph), but tracks
 * two things per function instead of one:
 *
 *   1. At most one input Class/Object/SoftClass/SoftObject parameter -- the "type picker" that a
 *      caller's wired-in class determines the output type from (native equivalent: the parameter
 *      named by meta=(DeterminesOutputType="ParamName")).
 *   2. Zero or more output Object/Class/Interface/SoftObject/SoftClass parameters that get retyped
 *      based on the picker (native equivalent: meta=(DynamicOutputParam="Param1,Param2")). If none are
 *      flagged, the function's return value is retyped by default -- matching native behavior exactly.
 */
class FViperToolsDynamicOutputMetadata
{
public:
	/** The input parameter flagged as the type picker for this function's graph, or NAME_None if unset. */
	static FName GetTypePickerParam(const class UEdGraph* Graph);

	/** Sets (or clears, passing NAME_None) the type picker parameter for this function's graph. Only one is allowed, so this replaces any previous picker. */
	static void SetTypePickerParam(class UEdGraph* Graph, FName ParamName);

	/** Returns the set of output parameter names currently flagged as dynamic outputs for this function's graph. */
	static TArray<FName> GetDynamicOutputParams(const class UEdGraph* Graph);

	/** Returns true if ParamName is currently flagged as a dynamic output on this function's graph. */
	static bool IsDynamicOutputParam(const class UEdGraph* Graph, FName ParamName);

	/** Flags or unflags ParamName as a dynamic output on this function's graph. */
	static void SetDynamicOutputParam(class UEdGraph* Graph, FName ParamName, bool bFlagged);

private:
	static const FName TypePickerKey;
	static const FName DynamicOutputParamsKey;
};

// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Persistent storage for "which enum parameters on this Blueprint function should be expanded into exec
 * pins" -- completely separate from any user-facing field like Keywords, Category, or ToolTip.
 *
 * Storage mechanism: UMetaData attached directly to the function's UEdGraph object. UMetaData is the
 * same general-purpose, per-object, editor-only key/value store the engine itself uses for every
 * meta=(...) specifier -- it's a first-class, sanctioned way to attach arbitrary custom tags to any
 * UObject (including a function's graph) without touching any UI field that already has its own
 * documented purpose. It persists with the asset when saved and requires no engine modification.
 */
class FViperToolsExpandExecsMetadata
{
public:
	/** Returns the set of parameter names currently flagged to expand as exec pins on this function's graph. */
	static TArray<FName> GetFlaggedParams(const class UEdGraph* Graph);

	/** Returns true if ParamName is currently flagged on this function's graph. */
	static bool IsParamFlagged(const class UEdGraph* Graph, FName ParamName);

	/** Flags or unflags ParamName on this function's graph, and marks the package dirty if changed. */
	static void SetParamFlagged(class UEdGraph* Graph, FName ParamName, bool bFlagged);

private:
	static const FName MetaKey;
};

// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Persistent storage for "which parameters on this Blueprint function should be hidden behind the
 * advanced-pin expand arrow" -- mirrors FViperToolsExpandExecsMetadata's approach (UMetaData on the
 * function's UEdGraph). Native equivalent: a UPARAM/parameter marked AdvancedDisplay in C++, which
 * sets the CPF_AdvancedDisplay property flag (see FViperToolsAdvancedDisplaySync).
 */
class FViperToolsAdvancedDisplayMetadata
{
public:
	/** Returns the set of parameter names currently flagged as advanced/hidden on this function's graph. */
	static TArray<FName> GetFlaggedParams(const class UEdGraph* Graph);

	/** Returns true if ParamName is currently flagged on this function's graph. */
	static bool IsParamFlagged(const class UEdGraph* Graph, FName ParamName);

	/** Flags or unflags ParamName on this function's graph, and marks the package dirty if changed. */
	static void SetParamFlagged(class UEdGraph* Graph, FName ParamName, bool bFlagged);

private:
	static const FName MetaKey;
};

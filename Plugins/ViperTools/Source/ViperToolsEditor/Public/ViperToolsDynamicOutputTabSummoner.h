// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FBlueprintEditor;

/** Registers the "Dynamic Output Types" tab (SViperToolsDynamicOutputPanel) inside the Blueprint function editor. */
class FViperToolsDynamicOutputTabSummoner : public FWorkflowTabFactory
{
public:
	static const FName TabId;

	FViperToolsDynamicOutputTabSummoner(TSharedPtr<FBlueprintEditor> InBlueprintEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

private:
	TWeakPtr<FBlueprintEditor> BlueprintEditorWeak;
};

// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FBlueprintEditor;

/** Registers the "Viper Tools" tab (SViperToolsExecExpanderPanel) inside the Blueprint function editor. */
class FViperToolsExecExpanderTabSummoner : public FWorkflowTabFactory
{
public:
	static const FName TabId;

	FViperToolsExecExpanderTabSummoner(TSharedPtr<FBlueprintEditor> InBlueprintEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

private:
	TWeakPtr<FBlueprintEditor> BlueprintEditorWeak;
};

// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsDefaultToSelfMetadata.h"
#include "EdGraph/EdGraph.h"
#include "UObject/Package.h"
#include "UObject/MetaData.h"

const FName FViperToolsDefaultToSelfMetadata::MetaKey(TEXT("VT_DefaultToSelf"));

FName FViperToolsDefaultToSelfMetadata::GetDefaultToSelfParam(const UEdGraph* Graph)
{
	if (!Graph)
	{
		return NAME_None;
	}

	UPackage* Package = Graph->GetOutermost();
	UMetaData* MetaDataObj = Package ? Package->GetMetaData() : nullptr;
	if (!MetaDataObj || !MetaDataObj->HasValue(Graph, MetaKey))
	{
		return NAME_None;
	}

	const FString Value = MetaDataObj->GetValue(Graph, MetaKey);
	return Value.IsEmpty() ? NAME_None : FName(*Value);
}

void FViperToolsDefaultToSelfMetadata::SetDefaultToSelfParam(UEdGraph* Graph, FName ParamName)
{
	if (!Graph)
	{
		return;
	}

	UPackage* Package = Graph->GetOutermost();
	UMetaData* MetaDataObj = Package ? Package->GetMetaData() : nullptr;
	if (!MetaDataObj)
	{
		return;
	}

	if (GetDefaultToSelfParam(Graph) == ParamName)
	{
		return;
	}

	Graph->Modify();

	if (ParamName.IsNone())
	{
		MetaDataObj->RemoveValue(Graph, MetaKey);
	}
	else
	{
		MetaDataObj->SetValue(Graph, MetaKey, *ParamName.ToString());
	}

	Package->MarkPackageDirty();
}

// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsAdvancedDisplayMetadata.h"
#include "EdGraph/EdGraph.h"
#include "UObject/Package.h"
#include "UObject/MetaData.h"

const FName FViperToolsAdvancedDisplayMetadata::MetaKey(TEXT("VT_AdvancedDisplayParams"));

TArray<FName> FViperToolsAdvancedDisplayMetadata::GetFlaggedParams(const UEdGraph* Graph)
{
	TArray<FName> Result;

	if (!Graph)
	{
		return Result;
	}

	UPackage* Package = Graph->GetOutermost();
	if (!Package)
	{
		return Result;
	}

	UMetaData* MetaDataObj = Package->GetMetaData();
	if (!MetaDataObj || !MetaDataObj->HasValue(Graph, MetaKey))
	{
		return Result;
	}

	const FString Value = MetaDataObj->GetValue(Graph, MetaKey);

	TArray<FString> Parts;
	Value.ParseIntoArray(Parts, TEXT(","), true);

	Result.Reserve(Parts.Num());
	for (const FString& Part : Parts)
	{
		Result.Add(FName(*Part.TrimStartAndEnd()));
	}

	return Result;
}

bool FViperToolsAdvancedDisplayMetadata::IsParamFlagged(const UEdGraph* Graph, FName ParamName)
{
	return GetFlaggedParams(Graph).Contains(ParamName);
}

void FViperToolsAdvancedDisplayMetadata::SetParamFlagged(UEdGraph* Graph, FName ParamName, bool bFlagged)
{
	if (!Graph)
	{
		return;
	}

	TArray<FName> Current = GetFlaggedParams(Graph);
	const bool bAlreadyFlagged = Current.Contains(ParamName);

	if (bFlagged == bAlreadyFlagged)
	{
		return;
	}

	if (bFlagged)
	{
		Current.Add(ParamName);
	}
	else
	{
		Current.Remove(ParamName);
	}

	UPackage* Package = Graph->GetOutermost();
	if (!Package)
	{
		return;
	}

	UMetaData* MetaDataObj = Package->GetMetaData();
	if (!MetaDataObj)
	{
		return;
	}

	Graph->Modify();

	if (Current.Num() == 0)
	{
		MetaDataObj->RemoveValue(Graph, MetaKey);
	}
	else
	{
		TArray<FString> Strings;
		Strings.Reserve(Current.Num());
		for (const FName& Name : Current)
		{
			Strings.Add(Name.ToString());
		}
		MetaDataObj->SetValue(Graph, MetaKey, *FString::Join(Strings, TEXT(",")));
	}

	Package->MarkPackageDirty();
}

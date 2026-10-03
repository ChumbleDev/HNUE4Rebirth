// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsDynamicOutputMetadata.h"
#include "EdGraph/EdGraph.h"
#include "UObject/Package.h"
#include "UObject/MetaData.h"

const FName FViperToolsDynamicOutputMetadata::TypePickerKey(TEXT("VT_DeterminesOutputType"));
const FName FViperToolsDynamicOutputMetadata::DynamicOutputParamsKey(TEXT("VT_DynamicOutputParams"));

namespace
{
	TArray<FName> ParseCommaList(const FString& Value)
	{
		TArray<FName> Result;
		TArray<FString> Parts;
		Value.ParseIntoArray(Parts, TEXT(","), true);
		Result.Reserve(Parts.Num());
		for (const FString& Part : Parts)
		{
			Result.Add(FName(*Part.TrimStartAndEnd()));
		}
		return Result;
	}

	FString JoinNames(const TArray<FName>& Names)
	{
		TArray<FString> Strings;
		Strings.Reserve(Names.Num());
		for (const FName& Name : Names)
		{
			Strings.Add(Name.ToString());
		}
		return FString::Join(Strings, TEXT(","));
	}
}

FName FViperToolsDynamicOutputMetadata::GetTypePickerParam(const UEdGraph* Graph)
{
	if (!Graph)
	{
		return NAME_None;
	}

	UPackage* Package = Graph->GetOutermost();
	UMetaData* MetaDataObj = Package ? Package->GetMetaData() : nullptr;
	if (!MetaDataObj || !MetaDataObj->HasValue(Graph, TypePickerKey))
	{
		return NAME_None;
	}

	const FString Value = MetaDataObj->GetValue(Graph, TypePickerKey);
	return Value.IsEmpty() ? NAME_None : FName(*Value);
}

void FViperToolsDynamicOutputMetadata::SetTypePickerParam(UEdGraph* Graph, FName ParamName)
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

	if (GetTypePickerParam(Graph) == ParamName)
	{
		return;
	}

	Graph->Modify();

	if (ParamName.IsNone())
	{
		MetaDataObj->RemoveValue(Graph, TypePickerKey);
	}
	else
	{
		MetaDataObj->SetValue(Graph, TypePickerKey, *ParamName.ToString());
	}

	Package->MarkPackageDirty();
}

TArray<FName> FViperToolsDynamicOutputMetadata::GetDynamicOutputParams(const UEdGraph* Graph)
{
	if (!Graph)
	{
		return TArray<FName>();
	}

	UPackage* Package = Graph->GetOutermost();
	UMetaData* MetaDataObj = Package ? Package->GetMetaData() : nullptr;
	if (!MetaDataObj || !MetaDataObj->HasValue(Graph, DynamicOutputParamsKey))
	{
		return TArray<FName>();
	}

	return ParseCommaList(MetaDataObj->GetValue(Graph, DynamicOutputParamsKey));
}

bool FViperToolsDynamicOutputMetadata::IsDynamicOutputParam(const UEdGraph* Graph, FName ParamName)
{
	return GetDynamicOutputParams(Graph).Contains(ParamName);
}

void FViperToolsDynamicOutputMetadata::SetDynamicOutputParam(UEdGraph* Graph, FName ParamName, bool bFlagged)
{
	if (!Graph)
	{
		return;
	}

	TArray<FName> Current = GetDynamicOutputParams(Graph);
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
	UMetaData* MetaDataObj = Package ? Package->GetMetaData() : nullptr;
	if (!MetaDataObj)
	{
		return;
	}

	Graph->Modify();

	if (Current.Num() == 0)
	{
		MetaDataObj->RemoveValue(Graph, DynamicOutputParamsKey);
	}
	else
	{
		MetaDataObj->SetValue(Graph, DynamicOutputParamsKey, *JoinNames(Current));
	}

	Package->MarkPackageDirty();
}

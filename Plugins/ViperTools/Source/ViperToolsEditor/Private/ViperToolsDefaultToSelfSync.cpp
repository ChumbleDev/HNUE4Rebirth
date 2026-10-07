// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsDefaultToSelfSync.h"
#include "ViperToolsDefaultToSelfMetadata.h"
#include "ViperToolsEditor.h"
#include "Engine/Blueprint.h"
#include "EdGraphSchema_K2.h"
#include "UObject/UnrealType.h"

namespace
{
	bool IsSelfLikeProperty(FProperty* Property)
	{
		return Property && (Property->IsA<FObjectProperty>() || Property->IsA<FInterfaceProperty>());
	}
}

void FViperToolsDefaultToSelfSync::ApplyToFunction(UFunction* Function, FName ParamName)
{
	if (!Function)
	{
		return;
	}

	bool bValid = false;
	if (!ParamName.IsNone())
	{
		FProperty* Param = Function->FindPropertyByName(ParamName);
		bValid = IsSelfLikeProperty(Param);
	}

	if (bValid)
	{
		Function->SetMetaData(FBlueprintMetadata::MD_DefaultToSelf, *ParamName.ToString());
	}
	else
	{
		Function->RemoveMetaData(FBlueprintMetadata::MD_DefaultToSelf);
	}
}

void FViperToolsDefaultToSelfSync::SyncBlueprint(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return;
	}

	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		const FName ParamName = FViperToolsDefaultToSelfMetadata::GetDefaultToSelfParam(Graph);
		const FName FunctionName = Graph->GetFName();

		if (Blueprint->GeneratedClass)
		{
			ApplyToFunction(Blueprint->GeneratedClass->FindFunctionByName(FunctionName), ParamName);
		}
		if (Blueprint->SkeletonGeneratedClass)
		{
			ApplyToFunction(Blueprint->SkeletonGeneratedClass->FindFunctionByName(FunctionName), ParamName);
		}
	}
}

void FViperToolsDefaultToSelfSync::SyncAllBlueprints()
{
	for (TObjectIterator<UBlueprint> It; It; ++It)
	{
		UBlueprint* Blueprint = *It;
		if (IsValid(Blueprint))
		{
			SyncBlueprint(Blueprint);
		}
	}
}

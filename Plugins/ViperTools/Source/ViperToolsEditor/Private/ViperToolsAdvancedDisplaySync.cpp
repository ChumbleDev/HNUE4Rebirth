// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsAdvancedDisplaySync.h"
#include "ViperToolsAdvancedDisplayMetadata.h"
#include "ViperToolsEditor.h"
#include "Engine/Blueprint.h"
#include "UObject/UnrealType.h"

void FViperToolsAdvancedDisplaySync::ApplyToFunction(UFunction* Function, const TArray<FName>& FlaggedParams)
{
	if (!Function)
	{
		return;
	}

	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		FProperty* Param = *It;
		if (!Param || !Param->HasAnyPropertyFlags(CPF_Parm) || Param->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			// Only real input/output parameters are eligible -- the return value pin always has a
			// fixed, visible position on the node and isn't a sensible candidate to hide.
			continue;
		}

		if (FlaggedParams.Contains(Param->GetFName()))
		{
			Param->SetPropertyFlags(CPF_AdvancedDisplay);
		}
		else
		{
			Param->ClearPropertyFlags(CPF_AdvancedDisplay);
		}
	}
}

void FViperToolsAdvancedDisplaySync::SyncBlueprint(UBlueprint* Blueprint)
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

		const TArray<FName> FlaggedParams = FViperToolsAdvancedDisplayMetadata::GetFlaggedParams(Graph);
		const FName FunctionName = Graph->GetFName();

		if (Blueprint->GeneratedClass)
		{
			ApplyToFunction(Blueprint->GeneratedClass->FindFunctionByName(FunctionName), FlaggedParams);
		}
		if (Blueprint->SkeletonGeneratedClass)
		{
			ApplyToFunction(Blueprint->SkeletonGeneratedClass->FindFunctionByName(FunctionName), FlaggedParams);
		}
	}
}

void FViperToolsAdvancedDisplaySync::SyncAllBlueprints()
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

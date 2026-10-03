// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsExpandEnumAsExecsSync.h"
#include "ViperToolsExpandExecsMetadata.h"
#include "ViperToolsEditor.h"
#include "Engine/Blueprint.h"
#include "EdGraphSchema_K2.h"
#include "UObject/UnrealType.h"
#include "K2Node_CallFunction.h"

void FViperToolsExpandEnumAsExecsSync::ApplyToFunction(UFunction* Function, const TArray<FName>& FlaggedParams)
{
	if (!Function)
	{
		return;
	}

	TArray<FString> ValidNames;
	ValidNames.Reserve(FlaggedParams.Num());

	for (const FName& ParamName : FlaggedParams)
	{
		FProperty* Param = Function->FindPropertyByName(ParamName);
		if (!Param)
		{
			continue;
		}

		const FByteProperty* ByteParam = CastField<FByteProperty>(Param);
		const bool bIsEnumParam = Param->IsA<FEnumProperty>() || (ByteParam && ByteParam->Enum != nullptr);
		if (bIsEnumParam)
		{
			ValidNames.Add(ParamName.ToString());
		}
	}

	if (ValidNames.Num() > 0)
	{
		Function->SetMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs, *FString::Join(ValidNames, TEXT(",")));
	}
	else
	{
		Function->RemoveMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs);
	}
}

void FViperToolsExpandEnumAsExecsSync::SyncBlueprint(UBlueprint* Blueprint)
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

		const TArray<FName> FlaggedParams = FViperToolsExpandExecsMetadata::GetFlaggedParams(Graph);
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

	FixUpEnumExecPinLabels(Blueprint);
}

void FViperToolsExpandEnumAsExecsSync::SyncAllBlueprints()
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

void FViperToolsExpandEnumAsExecsSync::FixUpEnumExecPinLabels(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return;
	}

	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		FixUpEnumExecPinLabelsInGraph(Graph);
	}
}

void FViperToolsExpandEnumAsExecsSync::FixUpEnumExecPinLabelsInGraph(UEdGraph* Graph)
{
	if (!Graph)
	{
		return;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Node))
		{
			FixUpEnumExecPinLabelsOnNode(CallFunctionNode);
		}
	}
}

void FViperToolsExpandEnumAsExecsSync::FixUpEnumExecPinLabelsOnNode(UK2Node_CallFunction* CallFunctionNode)
{
	if (!CallFunctionNode)
	{
		return;
	}

	const UFunction* TargetFunction = CallFunctionNode->GetTargetFunction();
	if (!TargetFunction)
	{
		return;
	}
	if (!TargetFunction->HasMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs))
	{
		return;
	}

	TArray<FString> EnumParamNameStrings;
	TargetFunction->GetMetaData(FBlueprintMetadata::MD_ExpandEnumAsExecs).ParseIntoArray(EnumParamNameStrings, TEXT(","));

	UE_LOG(LogViperTools, Warning, TEXT("[ExpandEnumLabels] Node '%s' (function '%s') has %d flagged enum param(s), %d pins on node"),
		*CallFunctionNode->GetName(), *TargetFunction->GetName(), EnumParamNameStrings.Num(), CallFunctionNode->Pins.Num());

	bool bChangedAny = false;

	for (const FString& EnumParamNameString : EnumParamNameStrings)
	{
		const FName EnumParamName(*EnumParamNameString);

		FProperty* Prop = nullptr;
		UEnum* Enum = nullptr;

		if (const FByteProperty* ByteProp = FindFProperty<FByteProperty>(TargetFunction, EnumParamName))
		{
			Prop = const_cast<FByteProperty*>(ByteProp);
			Enum = ByteProp->Enum;
		}
		else if (const FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(TargetFunction, EnumParamName))
		{
			Prop = const_cast<FEnumProperty*>(EnumProp);
			Enum = EnumProp->GetEnum();
		}

		UE_LOG(LogViperTools, Warning, TEXT("[ExpandEnumLabels] Param '%s' -> Enum %s (class: %s), NumEnums=%d"),
			*EnumParamNameString,
			Enum ? *Enum->GetName() : TEXT("NOT FOUND"),
			Enum ? *Enum->GetClass()->GetName() : TEXT("n/a"),
			Enum ? Enum->NumEnums() : -1);

		if (!Prop || !Enum)
		{
			continue;
		}

		// IMPORTANT: this must match pins by direction, not just by name. When a function has BOTH an
		// input-flagged enum param and an output-flagged enum param, their entries can produce
		// identically-named exec pins on each side (e.g. both using "NewEnumerator0"). Looking up by
		// name alone (ignoring direction) would grab whichever pin happens to come first regardless of
		// which side it's actually on, mislabeling the input exec pins with the output enum's text (or
		// vice versa). This formula is copied verbatim from the engine's own
		// UK2Node_CallFunction::AllocateDefaultPins -- the same logic that creates these exact pins in
		// the first place -- so it's guaranteed to classify consistently with however the engine
		// actually built them.
		const bool bIsFunctionInput = !Prop->HasAnyPropertyFlags(CPF_ReturnParm) &&
			(!Prop->HasAnyPropertyFlags(CPF_OutParm) || Prop->HasAnyPropertyFlags(CPF_ReferenceParm));
		const EEdGraphPinDirection Direction = bIsFunctionInput ? EGPD_Input : EGPD_Output;

		const int32 NumExecs = Enum->NumEnums() - 1;
		for (int32 ExecIdx = 0; ExecIdx < NumExecs; ExecIdx++)
		{
			if (Enum->HasMetaData(TEXT("Hidden"), ExecIdx) || Enum->HasMetaData(TEXT("Spacer"), ExecIdx))
			{
				continue;
			}

			const FString NameStr = Enum->GetNameStringByIndex(ExecIdx);
			UEdGraphPin* Pin = CallFunctionNode->FindPin(*NameStr, Direction);
			const FText NewFriendlyName = Enum->GetDisplayNameTextByIndex(ExecIdx);

			UE_LOG(LogViperTools, Warning, TEXT("[ExpandEnumLabels]   entry[%d] internal name '%s' -> display '%s' -- pin %s%s"),
				ExecIdx, *NameStr, *NewFriendlyName.ToString(),
				Pin ? TEXT("FOUND") : TEXT("NOT FOUND"),
				Pin ? *FString::Printf(TEXT(" (category=%s, current friendly='%s')"), *Pin->PinType.PinCategory.ToString(), *Pin->PinFriendlyName.ToString()) : TEXT(""));

			if (!Pin || Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
			{
				continue;
			}

			if (!Pin->PinFriendlyName.EqualTo(NewFriendlyName))
			{
				Pin->PinFriendlyName = NewFriendlyName;
				bChangedAny = true;
			}
		}
	}

	if (bChangedAny)
	{
		if (UPackage* Package = CallFunctionNode->GetOutermost())
		{
			Package->MarkPackageDirty();
		}
		CallFunctionNode->GetGraph()->NotifyGraphChanged();
	}
}

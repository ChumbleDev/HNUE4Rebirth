// Copyright Weavervilles. All Rights Reserved.

#include "ViperToolsDynamicOutputSync.h"
#include "ViperToolsDynamicOutputMetadata.h"
#include "ViperToolsEditor.h"
#include "Engine/Blueprint.h"
#include "EdGraphSchema_K2.h"
#include "UObject/UnrealType.h"
#include "K2Node_FunctionResult.h"

namespace
{
	// Named distinctly from the near-identical helper in SViperToolsDynamicOutputPanel.cpp -- both
	// live in anonymous namespaces, but Unreal's unity build merges multiple .cpp files into a single
	// translation unit, so two same-named functions in (nominally separate) anonymous namespaces still
	// collide at link/compile time.
	bool IsDynamicOutputCandidatePin(const UEdGraphPin* Pin)
	{
		if (!Pin || !Pin->PinType.PinSubCategoryObject.IsValid())
		{
			return false;
		}

		const FName Category = Pin->PinType.PinCategory;
		return Category == UEdGraphSchema_K2::PC_Object
			|| Category == UEdGraphSchema_K2::PC_Class
			|| Category == UEdGraphSchema_K2::PC_SoftObject
			|| Category == UEdGraphSchema_K2::PC_SoftClass
			|| Category == UEdGraphSchema_K2::PC_Interface;
	}

	bool IsObjectLikeProperty(FProperty* Property)
	{
		// Unwrap containers first -- an array/set/map of Object references is still a valid dynamic
		// output (e.g. TArray<BaseExtension*>), but its *outer* property is an FArrayProperty/FSetProperty/
		// FMapProperty, not an FObjectProperty, so the checks below must look at the element type instead.
		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			Property = ArrayProperty->Inner;
		}
		else if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
		{
			Property = SetProperty->ElementProp;
		}
		else if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			Property = MapProperty->ValueProp;
		}

		return Property &&
			(Property->IsA<FObjectProperty>() ||
			 Property->IsA<FClassProperty>() ||
			 Property->IsA<FSoftObjectProperty>() ||
			 Property->IsA<FSoftClassProperty>() ||
			 Property->IsA<FInterfaceProperty>());
	}
}

void FViperToolsDynamicOutputSync::ApplyToFunction(UFunction* Function, FName PickerParam, const TArray<FName>& DynamicOutputParams)
{
	if (!Function)
	{
		if (!PickerParam.IsNone())
		{
			UE_LOG(LogViperTools, Warning, TEXT("[DynamicOutput] ApplyToFunction called with a null Function for picker param '%s' -- FindFunctionByName failed to resolve this function on its class."), *PickerParam.ToString());
		}
		return;
	}

	bool bPickerValid = false;
	if (!PickerParam.IsNone())
	{
		FProperty* PickerProp = Function->FindPropertyByName(PickerParam);
		bPickerValid = IsObjectLikeProperty(PickerProp);
		UE_LOG(LogViperTools, Warning, TEXT("[DynamicOutput] Function '%s': picker param '%s' -> property %s (class: %s), valid=%s"),
			*Function->GetName(), *PickerParam.ToString(),
			PickerProp ? TEXT("FOUND") : TEXT("NOT FOUND"),
			PickerProp ? *PickerProp->GetClass()->GetName() : TEXT("n/a"),
			bPickerValid ? TEXT("true") : TEXT("false"));
	}

	if (bPickerValid)
	{
		Function->SetMetaData(FBlueprintMetadata::MD_DynamicOutputType, *PickerParam.ToString());
	}
	else
	{
		Function->RemoveMetaData(FBlueprintMetadata::MD_DynamicOutputType);
	}

	TArray<FString> ValidOutputs;
	if (bPickerValid)
	{
		ValidOutputs.Reserve(DynamicOutputParams.Num());
		for (const FName& ParamName : DynamicOutputParams)
		{
			FProperty* OutputProp = Function->FindPropertyByName(ParamName);
			const bool bOutputValid = IsObjectLikeProperty(OutputProp);
			UE_LOG(LogViperTools, Warning, TEXT("[DynamicOutput] Function '%s': dynamic output param '%s' -> property %s (class: %s), valid=%s"),
				*Function->GetName(), *ParamName.ToString(),
				OutputProp ? TEXT("FOUND") : TEXT("NOT FOUND"),
				OutputProp ? *OutputProp->GetClass()->GetName() : TEXT("n/a"),
				bOutputValid ? TEXT("true") : TEXT("false"));
			if (bOutputValid)
			{
				ValidOutputs.Add(ParamName.ToString());
			}
		}
	}

	if (ValidOutputs.Num() > 0)
	{
		Function->SetMetaData(FBlueprintMetadata::MD_DynamicOutputParam, *FString::Join(ValidOutputs, TEXT(",")));
	}
	else
	{
		Function->RemoveMetaData(FBlueprintMetadata::MD_DynamicOutputParam);
	}

	if (!PickerParam.IsNone())
	{
		UE_LOG(LogViperTools, Warning, TEXT("[DynamicOutput] Function '%s' final metadata -- DeterminesOutputType=\"%s\" DynamicOutputParam=\"%s\""),
			*Function->GetName(),
			*Function->GetMetaData(FBlueprintMetadata::MD_DynamicOutputType),
			*Function->GetMetaData(FBlueprintMetadata::MD_DynamicOutputParam));
	}
}

TArray<FName> FViperToolsDynamicOutputSync::ComputeImplicitDynamicOutputParams(UEdGraph* Graph)
{
	TArray<FName> Candidates;
	if (!Graph)
	{
		return Candidates;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
		{
			for (UEdGraphPin* Pin : ResultNode->Pins)
			{
				if (Pin->Direction == EGPD_Input && IsDynamicOutputCandidatePin(Pin))
				{
					Candidates.AddUnique(Pin->GetFName());
				}
			}
		}
	}

	// Only safe to pick one implicitly if there's exactly one candidate -- with multiple outputs the
	// user needs to explicitly flag which one(s) via the panel, same as native DynamicOutputParam.
	return Candidates.Num() == 1 ? Candidates : TArray<FName>();
}

void FViperToolsDynamicOutputSync::SyncBlueprint(UBlueprint* Blueprint)
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

		const FName PickerParam = FViperToolsDynamicOutputMetadata::GetTypePickerParam(Graph);
		TArray<FName> DynamicOutputParams = FViperToolsDynamicOutputMetadata::GetDynamicOutputParams(Graph);
		if (!PickerParam.IsNone() && DynamicOutputParams.Num() == 0)
		{
			DynamicOutputParams = ComputeImplicitDynamicOutputParams(Graph);
		}
		const FName FunctionName = Graph->GetFName();

		if (!PickerParam.IsNone())
		{
			UE_LOG(LogViperTools, Warning, TEXT("[DynamicOutput] Blueprint '%s' graph '%s': stored picker param = '%s', %d stored dynamic output param(s)"),
				*Blueprint->GetName(), *FunctionName.ToString(), *PickerParam.ToString(), DynamicOutputParams.Num());
		}

		if (Blueprint->GeneratedClass)
		{
			ApplyToFunction(Blueprint->GeneratedClass->FindFunctionByName(FunctionName), PickerParam, DynamicOutputParams);
		}
		if (Blueprint->SkeletonGeneratedClass)
		{
			ApplyToFunction(Blueprint->SkeletonGeneratedClass->FindFunctionByName(FunctionName), PickerParam, DynamicOutputParams);
		}
	}
}

void FViperToolsDynamicOutputSync::SyncAllBlueprints()
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

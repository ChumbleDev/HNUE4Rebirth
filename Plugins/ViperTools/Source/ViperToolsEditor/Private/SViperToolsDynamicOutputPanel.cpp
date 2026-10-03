// Copyright Weavervilles. All Rights Reserved.

#include "SViperToolsDynamicOutputPanel.h"
#include "ViperToolsDynamicOutputMetadata.h"
#include "BlueprintEditor.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "EdGraphSchema_K2.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "SViperToolsDynamicOutputPanel"

namespace
{
	bool IsClassOrObjectPickerPin(const UEdGraphPin* Pin)
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
}

void SViperToolsDynamicOutputPanel::Construct(const FArguments& InArgs, TWeakPtr<FBlueprintEditor> InBlueprintEditor)
{
	BlueprintEditorWeak = InBlueprintEditor;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Intro", "Pick one input (Class/Object) as the type picker, and optionally flag specific outputs to retype based on it -- leave outputs unflagged to default to Return Value, same as native DeterminesOutputType."))
				.AutoWrapText(true)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "Refresh"))
				.ToolTipText(LOCTEXT("RefreshTooltip", "Rescans this Blueprint's functions for Class/Object parameters. Use this after adding/removing a function or a parameter."))
				.OnClicked(this, &SViperToolsDynamicOutputPanel::OnRefreshClicked)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(RowsContainer, SVerticalBox)
			]
		]
	];

	RebuildRows();
}

FReply SViperToolsDynamicOutputPanel::OnRefreshClicked()
{
	RebuildRows();
	return FReply::Handled();
}

void SViperToolsDynamicOutputPanel::RebuildRows()
{
	if (!RowsContainer.IsValid())
	{
		return;
	}

	RowsContainer->ClearChildren();

	TSharedPtr<FBlueprintEditor> BlueprintEditor = BlueprintEditorWeak.Pin();
	UBlueprint* Blueprint = BlueprintEditor.IsValid() ? BlueprintEditor->GetBlueprintObj() : nullptr;

	if (!Blueprint)
	{
		RowsContainer->AddSlot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("NoBlueprint", "No Blueprint is currently open."))
			];
		return;
	}

	bool bFoundAny = false;

	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		TArray<UEdGraphPin*> InputCandidates;
		TArray<UEdGraphPin*> OutputCandidates;

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
			{
				for (UEdGraphPin* Pin : EntryNode->Pins)
				{
					if (Pin->Direction == EGPD_Output && IsClassOrObjectPickerPin(Pin))
					{
						InputCandidates.Add(Pin);
					}
				}
			}
			else if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
			{
				for (UEdGraphPin* Pin : ResultNode->Pins)
				{
					if (Pin->Direction == EGPD_Input && IsClassOrObjectPickerPin(Pin))
					{
						OutputCandidates.Add(Pin);
					}
				}
			}
		}

		if (InputCandidates.Num() == 0)
		{
			// Nothing a picker could ever be set to for this function -- skip it entirely.
			continue;
		}

		bFoundAny = true;

		RowsContainer->AddSlot()
			.AutoHeight()
			.Padding(4.0f, 8.0f, 4.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromName(Graph->GetFName()))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			];

		TWeakObjectPtr<UEdGraph> WeakGraph(Graph);

		RowsContainer->AddSlot()
			.AutoHeight()
			.Padding(16.0f, 2.0f, 4.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TypePickerHeader", "Type Picker Input (choose at most one):"))
			];

		for (UEdGraphPin* Pin : InputCandidates)
		{
			const FName ParamName = Pin->GetFName();

			RowsContainer->AddSlot()
				.AutoHeight()
				.Padding(32.0f, 1.0f, 4.0f, 1.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SViperToolsDynamicOutputPanel::GetPickerCheckState, WeakGraph, ParamName)
						.OnCheckStateChanged(this, &SViperToolsDynamicOutputPanel::OnPickerCheckStateChanged, WeakGraph, ParamName)
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromName(ParamName))
					]
				];
		}

		RowsContainer->AddSlot()
			.AutoHeight()
			.Padding(16.0f, 4.0f, 4.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DynamicOutputsHeader", "Dynamic Outputs (optional, defaults to Return Value):"))
			];

		if (OutputCandidates.Num() == 0)
		{
			RowsContainer->AddSlot()
				.AutoHeight()
				.Padding(32.0f, 1.0f, 4.0f, 1.0f)
				[
					SNew(STextBlock).Text(LOCTEXT("NoOutputCandidates", "No Class/Object output parameters found (the Return Value pin, if any, will be used by default)."))
				];
		}
		else
		{
			for (UEdGraphPin* Pin : OutputCandidates)
			{
				const FName ParamName = Pin->GetFName();

				RowsContainer->AddSlot()
					.AutoHeight()
					.Padding(32.0f, 1.0f, 4.0f, 1.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SCheckBox)
							.IsChecked(this, &SViperToolsDynamicOutputPanel::GetOutputCheckState, WeakGraph, ParamName)
							.OnCheckStateChanged(this, &SViperToolsDynamicOutputPanel::OnOutputCheckStateChanged, WeakGraph, ParamName)
						]
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromName(ParamName))
						]
					];
			}
		}
	}

	if (!bFoundAny)
	{
		RowsContainer->AddSlot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("NoCandidates", "No functions with a Class/Object/Interface input parameter found. Add one, compile, then hit Refresh."))
			];
	}
}

ECheckBoxState SViperToolsDynamicOutputPanel::GetPickerCheckState(TWeakObjectPtr<UEdGraph> Graph, FName ParamName) const
{
	UEdGraph* GraphPtr = Graph.Get();
	if (!GraphPtr)
	{
		return ECheckBoxState::Unchecked;
	}
	return FViperToolsDynamicOutputMetadata::GetTypePickerParam(GraphPtr) == ParamName ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SViperToolsDynamicOutputPanel::OnPickerCheckStateChanged(ECheckBoxState NewState, TWeakObjectPtr<UEdGraph> Graph, FName ParamName)
{
	UEdGraph* GraphPtr = Graph.Get();
	if (!GraphPtr)
	{
		return;
	}

	if (NewState == ECheckBoxState::Checked)
	{
		FViperToolsDynamicOutputMetadata::SetTypePickerParam(GraphPtr, ParamName);
	}
	else if (FViperToolsDynamicOutputMetadata::GetTypePickerParam(GraphPtr) == ParamName)
	{
		FViperToolsDynamicOutputMetadata::SetTypePickerParam(GraphPtr, NAME_None);
	}
}

ECheckBoxState SViperToolsDynamicOutputPanel::GetOutputCheckState(TWeakObjectPtr<UEdGraph> Graph, FName ParamName) const
{
	UEdGraph* GraphPtr = Graph.Get();
	if (!GraphPtr)
	{
		return ECheckBoxState::Unchecked;
	}
	return FViperToolsDynamicOutputMetadata::IsDynamicOutputParam(GraphPtr, ParamName) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SViperToolsDynamicOutputPanel::OnOutputCheckStateChanged(ECheckBoxState NewState, TWeakObjectPtr<UEdGraph> Graph, FName ParamName)
{
	UEdGraph* GraphPtr = Graph.Get();
	if (!GraphPtr)
	{
		return;
	}
	FViperToolsDynamicOutputMetadata::SetDynamicOutputParam(GraphPtr, ParamName, NewState == ECheckBoxState::Checked);
}

#undef LOCTEXT_NAMESPACE

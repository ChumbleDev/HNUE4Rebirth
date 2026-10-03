// Copyright Weavervilles. All Rights Reserved.

#include "SViperToolsExecExpanderPanel.h"
#include "ViperToolsExpandExecsMetadata.h"
#include "BlueprintEditor.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "EdGraphSchema_K2.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "SViperToolsExecExpanderPanel"

namespace
{
	struct FEnumParamRow
	{
		UEdGraphPin* Pin = nullptr;
		bool bIsFunctionInput = false;
	};

	bool IsEnumPin(const UEdGraphPin* Pin)
	{
		if (!Pin)
		{
			return false;
		}

		const bool bByteOrEnumCategory =
			Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Enum ||
			Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte;

		return bByteOrEnumCategory
			&& Pin->PinType.PinSubCategoryObject.IsValid()
			&& Pin->PinType.PinSubCategoryObject->IsA<UEnum>();
	}
}

void SViperToolsExecExpanderPanel::Construct(const FArguments& InArgs, TWeakPtr<FBlueprintEditor> InBlueprintEditor)
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
				.Text(LOCTEXT("Intro", "Check an enum input or output parameter below to render it as exec pins on this function's call node, just like native C++ ExpandEnumAsExecs."))
				.AutoWrapText(true)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "Refresh"))
				.ToolTipText(LOCTEXT("RefreshTooltip", "Rescans this Blueprint's functions for enum parameters. Use this after adding/removing a function or an enum parameter."))
				.OnClicked(this, &SViperToolsExecExpanderPanel::OnRefreshClicked)
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

FReply SViperToolsExecExpanderPanel::OnRefreshClicked()
{
	RebuildRows();
	return FReply::Handled();
}

void SViperToolsExecExpanderPanel::RebuildRows()
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

		TArray<FEnumParamRow> EnumRows;

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
			{
				for (UEdGraphPin* Pin : EntryNode->Pins)
				{
					if (IsEnumPin(Pin))
					{
						EnumRows.Add({ Pin, /*bIsFunctionInput=*/ true });
					}
				}
			}
			else if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
			{
				for (UEdGraphPin* Pin : ResultNode->Pins)
				{
					if (IsEnumPin(Pin))
					{
						EnumRows.Add({ Pin, /*bIsFunctionInput=*/ false });
					}
				}
			}
		}

		if (EnumRows.Num() == 0)
		{
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

		for (const FEnumParamRow& Row : EnumRows)
		{
			const FName ParamName = Row.Pin->GetFName();
			const FText DirectionLabel = Row.bIsFunctionInput ? LOCTEXT("InputParam", "Input") : LOCTEXT("OutputParam", "Output");

			RowsContainer->AddSlot()
				.AutoHeight()
				.Padding(16.0f, 1.0f, 4.0f, 1.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SViperToolsExecExpanderPanel::GetCheckState, WeakGraph, ParamName)
						.OnCheckStateChanged(this, &SViperToolsExecExpanderPanel::OnCheckStateChanged, WeakGraph, ParamName)
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("ParamRowFormat", "{0}  ({1})"), FText::FromName(ParamName), DirectionLabel))
					]
				];
		}
	}

	if (!bFoundAny)
	{
		RowsContainer->AddSlot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("NoEnumParams", "No enum-typed function parameters found. Add an Enum input or output to a custom function, compile, then hit Refresh."))
			];
	}
}

ECheckBoxState SViperToolsExecExpanderPanel::GetCheckState(TWeakObjectPtr<UEdGraph> Graph, FName ParamName) const
{
	UEdGraph* GraphPtr = Graph.Get();
	if (!GraphPtr)
	{
		return ECheckBoxState::Unchecked;
	}
	return FViperToolsExpandExecsMetadata::IsParamFlagged(GraphPtr, ParamName) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SViperToolsExecExpanderPanel::OnCheckStateChanged(ECheckBoxState NewState, TWeakObjectPtr<UEdGraph> Graph, FName ParamName)
{
	UEdGraph* GraphPtr = Graph.Get();
	if (!GraphPtr)
	{
		return;
	}
	FViperToolsExpandExecsMetadata::SetParamFlagged(GraphPtr, ParamName, NewState == ECheckBoxState::Checked);
}

#undef LOCTEXT_NAMESPACE

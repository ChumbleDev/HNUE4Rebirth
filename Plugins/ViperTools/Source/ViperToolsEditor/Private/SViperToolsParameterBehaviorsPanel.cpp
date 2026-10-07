// Copyright Weavervilles. All Rights Reserved.

#include "SViperToolsParameterBehaviorsPanel.h"
#include "ViperToolsExpandExecsMetadata.h"
#include "ViperToolsDynamicOutputMetadata.h"
#include "ViperToolsDefaultToSelfMetadata.h"
#include "ViperToolsAdvancedDisplayMetadata.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "SViperToolsParameterBehaviorsPanel"

void SViperToolsParameterBehaviorsPanel::Construct(const FArguments& InArgs)
{
	StatusText = LOCTEXT("StatusPickClass", "Pick a class, then type a function name and hit Find.");

	ChildSlot
	[
		SNew(SVerticalBox)

		// Intro
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Intro", "Configure Expandable Enum, Dynamic Output, Default To Self, and Advanced/Hidden Pin behaviors for any Blueprint function, from one place. Macros aren't supported -- they have no underlying UFunction for these behaviors to attach to."))
			.AutoWrapText(true)
		]

		// Class picker
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("ClassLabel", "Class:"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SClassPropertyEntryBox)
				.MetaClass(UObject::StaticClass())
				.AllowAbstract(true)
				.AllowNone(true)
				.SelectedClass(this, &SViperToolsParameterBehaviorsPanel::GetSelectedClass)
				.OnSetClass(this, &SViperToolsParameterBehaviorsPanel::OnClassPicked)
			]
		]

		// Function name + find button
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("FunctionLabel", "Function name (case-sensitive):"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SAssignNew(FunctionNameBox, SEditableTextBox)
				.OnTextChanged(this, &SViperToolsParameterBehaviorsPanel::OnFunctionNameTextChanged)
				.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type) { OnFindFunctionClicked(); })
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Find", "Find"))
				.OnClicked(this, &SViperToolsParameterBehaviorsPanel::OnFindFunctionClicked)
			]
		]

		// Status
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(this, &SViperToolsParameterBehaviorsPanel::GetStatusText)
			.AutoWrapText(true)
		]

		// Mode selector
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.0f, 6.0f, 6.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ModeLabel", "Behavior to configure:"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.0f, 0.0f, 6.0f, 4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.Style(FCoreStyle::Get(), "RadioButton")
					.IsChecked(this, &SViperToolsParameterBehaviorsPanel::GetModeCheckState, EViperParamBehaviorMode::ExpandAsExecs)
					.OnCheckStateChanged(this, &SViperToolsParameterBehaviorsPanel::OnModeCheckStateChanged, EViperParamBehaviorMode::ExpandAsExecs)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(GetModeLabel(EViperParamBehaviorMode::ExpandAsExecs)).ToolTipText(GetModeTooltip(EViperParamBehaviorMode::ExpandAsExecs))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.Style(FCoreStyle::Get(), "RadioButton")
					.IsChecked(this, &SViperToolsParameterBehaviorsPanel::GetModeCheckState, EViperParamBehaviorMode::OutputTypePicker)
					.OnCheckStateChanged(this, &SViperToolsParameterBehaviorsPanel::OnModeCheckStateChanged, EViperParamBehaviorMode::OutputTypePicker)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(GetModeLabel(EViperParamBehaviorMode::OutputTypePicker)).ToolTipText(GetModeTooltip(EViperParamBehaviorMode::OutputTypePicker))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.Style(FCoreStyle::Get(), "RadioButton")
					.IsChecked(this, &SViperToolsParameterBehaviorsPanel::GetModeCheckState, EViperParamBehaviorMode::DynamicOutputParam)
					.OnCheckStateChanged(this, &SViperToolsParameterBehaviorsPanel::OnModeCheckStateChanged, EViperParamBehaviorMode::DynamicOutputParam)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(GetModeLabel(EViperParamBehaviorMode::DynamicOutputParam)).ToolTipText(GetModeTooltip(EViperParamBehaviorMode::DynamicOutputParam))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.Style(FCoreStyle::Get(), "RadioButton")
					.IsChecked(this, &SViperToolsParameterBehaviorsPanel::GetModeCheckState, EViperParamBehaviorMode::DefaultToSelf)
					.OnCheckStateChanged(this, &SViperToolsParameterBehaviorsPanel::OnModeCheckStateChanged, EViperParamBehaviorMode::DefaultToSelf)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(GetModeLabel(EViperParamBehaviorMode::DefaultToSelf)).ToolTipText(GetModeTooltip(EViperParamBehaviorMode::DefaultToSelf))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.Style(FCoreStyle::Get(), "RadioButton")
					.IsChecked(this, &SViperToolsParameterBehaviorsPanel::GetModeCheckState, EViperParamBehaviorMode::AdvancedDisplay)
					.OnCheckStateChanged(this, &SViperToolsParameterBehaviorsPanel::OnModeCheckStateChanged, EViperParamBehaviorMode::AdvancedDisplay)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(GetModeLabel(EViperParamBehaviorMode::AdvancedDisplay)).ToolTipText(GetModeTooltip(EViperParamBehaviorMode::AdvancedDisplay))
				]
			]
		]

		// Parameter rows
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(6.0f, 0.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(RowsContainer, SVerticalBox)
			]
		]

		// Apply
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(6.0f)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("Apply", "Apply Changes (Compile Blueprint)"))
			.OnClicked(this, &SViperToolsParameterBehaviorsPanel::OnApplyClicked)
		]
	];

	RebuildParameterRows();
}

const UClass* SViperToolsParameterBehaviorsPanel::GetSelectedClass() const
{
	return SelectedClass.Get();
}

void SViperToolsParameterBehaviorsPanel::OnClassPicked(const UClass* NewClass)
{
	SelectedClass = NewClass;
	ResolvedBlueprint = nullptr;
	ResolvedGraph = nullptr;
	StatusText = LOCTEXT("StatusPickedClass", "Class selected. Type a function name and hit Find.");
	RebuildParameterRows();
}

void SViperToolsParameterBehaviorsPanel::OnFunctionNameTextChanged(const FText& NewText)
{
	FunctionNameInput = NewText.ToString();
}

FReply SViperToolsParameterBehaviorsPanel::OnFindFunctionClicked()
{
	ResolveFunction();
	RebuildParameterRows();
	return FReply::Handled();
}

void SViperToolsParameterBehaviorsPanel::ResolveFunction()
{
	ResolvedBlueprint = nullptr;
	ResolvedGraph = nullptr;

	const UClass* ClassPtr = SelectedClass.Get();
	if (!ClassPtr)
	{
		StatusText = LOCTEXT("StatusNoClass", "Pick a class first.");
		return;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(ClassPtr->ClassGeneratedBy);
	if (!Blueprint)
	{
		StatusText = LOCTEXT("StatusNotBlueprint", "That class isn't Blueprint-generated. Parameter Behaviors only supports Blueprint classes.");
		return;
	}

	if (FunctionNameInput.IsEmpty())
	{
		StatusText = LOCTEXT("StatusNoName", "Type a function name.");
		return;
	}

	UEdGraph* FoundGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph && Graph->GetFName().ToString().Equals(FunctionNameInput, ESearchCase::CaseSensitive))
		{
			FoundGraph = Graph;
			break;
		}
	}

	if (!FoundGraph)
	{
		// Give a clear, honest answer if the name instead matches a macro -- rather than silently
		// finding nothing with no explanation.
		for (UEdGraph* Graph : Blueprint->MacroGraphs)
		{
			if (Graph && Graph->GetFName().ToString().Equals(FunctionNameInput, ESearchCase::CaseSensitive))
			{
				StatusText = LOCTEXT("StatusIsMacro", "That name matches a macro, not a function. Macros aren't supported -- they have no underlying UFunction for these behaviors.");
				return;
			}
		}

		StatusText = FText::Format(LOCTEXT("StatusNotFound", "No function named \"{0}\" found on this Blueprint (exact case match required)."), FText::FromString(FunctionNameInput));
		return;
	}

	ResolvedBlueprint = Blueprint;
	ResolvedGraph = FoundGraph;
	StatusText = FText::Format(LOCTEXT("StatusFound", "Found function \"{0}\". Pick a behavior below and check its parameters."), FText::FromString(FunctionNameInput));
}

FText SViperToolsParameterBehaviorsPanel::GetStatusText() const
{
	return StatusText;
}

ECheckBoxState SViperToolsParameterBehaviorsPanel::GetModeCheckState(EViperParamBehaviorMode Mode) const
{
	return CurrentMode == Mode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SViperToolsParameterBehaviorsPanel::OnModeCheckStateChanged(ECheckBoxState NewState, EViperParamBehaviorMode Mode)
{
	if (NewState != ECheckBoxState::Checked)
	{
		return;
	}
	CurrentMode = Mode;
	RebuildParameterRows();
}

FText SViperToolsParameterBehaviorsPanel::GetModeLabel(EViperParamBehaviorMode Mode)
{
	switch (Mode)
	{
	case EViperParamBehaviorMode::ExpandAsExecs: return LOCTEXT("ModeExpandAsExecs", "Expandable Enum");
	case EViperParamBehaviorMode::OutputTypePicker: return LOCTEXT("ModeOutputTypePicker", "Dynamic Output: Type Picker");
	case EViperParamBehaviorMode::DynamicOutputParam: return LOCTEXT("ModeDynamicOutputParam", "Dynamic Output: Retyped Param");
	case EViperParamBehaviorMode::DefaultToSelf: return LOCTEXT("ModeDefaultToSelf", "Default To Self");
	case EViperParamBehaviorMode::AdvancedDisplay: return LOCTEXT("ModeAdvancedDisplay", "Advanced / Hidden Pin");
	default: return FText::GetEmpty();
	}
}

FText SViperToolsParameterBehaviorsPanel::GetModeTooltip(EViperParamBehaviorMode Mode)
{
	switch (Mode)
	{
	case EViperParamBehaviorMode::ExpandAsExecs:
		return LOCTEXT("ModeExpandAsExecsTip", "Expand an enum input/output parameter into exec pins, like native ExpandEnumAsExecs. Multiple parameters allowed.");
	case EViperParamBehaviorMode::OutputTypePicker:
		return LOCTEXT("ModeOutputTypePickerTip", "Pick one Class/Object input parameter whose wired-in value determines dynamic output types. Only one allowed.");
	case EViperParamBehaviorMode::DynamicOutputParam:
		return LOCTEXT("ModeDynamicOutputParamTip", "Flag Class/Object output parameters to retype based on the type picker. Leave unflagged to default to Return Value. Multiple parameters allowed.");
	case EViperParamBehaviorMode::DefaultToSelf:
		return LOCTEXT("ModeDefaultToSelfTip", "Pick one Object/Interface input parameter to hide and auto-wire to the calling graph's Self reference when left unconnected. Only one allowed.");
	case EViperParamBehaviorMode::AdvancedDisplay:
		return LOCTEXT("ModeAdvancedDisplayTip", "Hide input/output parameters behind the advanced-pin expand arrow, like native AdvancedDisplay (e.g. Lerp). Multiple parameters allowed.");
	default:
		return FText::GetEmpty();
	}
}

bool SViperToolsParameterBehaviorsPanel::IsEnumPin(const UEdGraphPin* Pin)
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

bool SViperToolsParameterBehaviorsPanel::IsObjectOrClassLikePin(const UEdGraphPin* Pin)
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

bool SViperToolsParameterBehaviorsPanel::IsSelfLikePin(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return false;
	}

	const FName Category = Pin->PinType.PinCategory;
	return Category == UEdGraphSchema_K2::PC_Object || Category == UEdGraphSchema_K2::PC_Interface;
}

bool SViperToolsParameterBehaviorsPanel::IsAdvancedDisplayCandidatePin(const UEdGraphPin* Pin)
{
	// Any real data parameter is eligible to be hidden behind the advanced-pin arrow -- exec pins
	// aren't, since they're flow control, not data.
	return Pin && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec;
}

void SViperToolsParameterBehaviorsPanel::GatherCandidatePins(EViperParamBehaviorMode Mode, TArray<UEdGraphPin*>& OutPins) const
{
	OutPins.Reset();

	UEdGraph* Graph = ResolvedGraph.Get();
	if (!Graph)
	{
		return;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
		{
			// On the Entry node, function INPUT parameters are the node's OUTPUT pins (they feed the
			// function body).
			for (UEdGraphPin* Pin : EntryNode->Pins)
			{
				if (Pin->Direction != EGPD_Output)
				{
					continue;
				}

				switch (Mode)
				{
				case EViperParamBehaviorMode::ExpandAsExecs:
					if (IsEnumPin(Pin)) OutPins.Add(Pin);
					break;
				case EViperParamBehaviorMode::OutputTypePicker:
					if (IsObjectOrClassLikePin(Pin)) OutPins.Add(Pin);
					break;
				case EViperParamBehaviorMode::DefaultToSelf:
					if (IsSelfLikePin(Pin)) OutPins.Add(Pin);
					break;
				case EViperParamBehaviorMode::AdvancedDisplay:
					if (IsAdvancedDisplayCandidatePin(Pin)) OutPins.Add(Pin);
					break;
				case EViperParamBehaviorMode::DynamicOutputParam:
				default:
					break;
				}
			}
		}
		else if (UK2Node_FunctionResult* ResultNode = Cast<UK2Node_FunctionResult>(Node))
		{
			// On the Result node, function OUTPUT parameters are the node's INPUT pins.
			for (UEdGraphPin* Pin : ResultNode->Pins)
			{
				if (Pin->Direction != EGPD_Input)
				{
					continue;
				}

				switch (Mode)
				{
				case EViperParamBehaviorMode::ExpandAsExecs:
					if (IsEnumPin(Pin)) OutPins.Add(Pin);
					break;
				case EViperParamBehaviorMode::DynamicOutputParam:
					if (IsObjectOrClassLikePin(Pin)) OutPins.Add(Pin);
					break;
				case EViperParamBehaviorMode::AdvancedDisplay:
					if (IsAdvancedDisplayCandidatePin(Pin)) OutPins.Add(Pin);
					break;
				case EViperParamBehaviorMode::OutputTypePicker:
				case EViperParamBehaviorMode::DefaultToSelf:
				default:
					break;
				}
			}
		}
	}
}

void SViperToolsParameterBehaviorsPanel::RebuildParameterRows()
{
	if (!RowsContainer.IsValid())
	{
		return;
	}

	RowsContainer->ClearChildren();

	UEdGraph* Graph = ResolvedGraph.Get();
	if (!Graph)
	{
		RowsContainer->AddSlot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("NoFunctionFound", "No function resolved yet."))
			];
		return;
	}

	TArray<UEdGraphPin*> Candidates;
	GatherCandidatePins(CurrentMode, Candidates);

	if (Candidates.Num() == 0)
	{
		RowsContainer->AddSlot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("NoCandidatesForMode", "No parameters on this function are eligible for the selected behavior."))
			];
		return;
	}

	for (UEdGraphPin* Pin : Candidates)
	{
		const FName ParamName = Pin->GetFName();
		const FText DirectionLabel = (Pin->Direction == EGPD_Output) ? LOCTEXT("InputParam", "Input") : LOCTEXT("OutputParam", "Output");

		RowsContainer->AddSlot()
			.AutoHeight()
			.Padding(4.0f, 1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked(this, &SViperToolsParameterBehaviorsPanel::GetParamCheckState, ParamName)
					.OnCheckStateChanged(this, &SViperToolsParameterBehaviorsPanel::OnParamCheckStateChanged, ParamName)
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

ECheckBoxState SViperToolsParameterBehaviorsPanel::GetParamCheckState(FName ParamName) const
{
	UEdGraph* Graph = ResolvedGraph.Get();
	if (!Graph)
	{
		return ECheckBoxState::Unchecked;
	}

	bool bChecked = false;
	switch (CurrentMode)
	{
	case EViperParamBehaviorMode::ExpandAsExecs:
		bChecked = FViperToolsExpandExecsMetadata::IsParamFlagged(Graph, ParamName);
		break;
	case EViperParamBehaviorMode::OutputTypePicker:
		bChecked = FViperToolsDynamicOutputMetadata::GetTypePickerParam(Graph) == ParamName;
		break;
	case EViperParamBehaviorMode::DynamicOutputParam:
		bChecked = FViperToolsDynamicOutputMetadata::IsDynamicOutputParam(Graph, ParamName);
		break;
	case EViperParamBehaviorMode::DefaultToSelf:
		bChecked = FViperToolsDefaultToSelfMetadata::GetDefaultToSelfParam(Graph) == ParamName;
		break;
	case EViperParamBehaviorMode::AdvancedDisplay:
		bChecked = FViperToolsAdvancedDisplayMetadata::IsParamFlagged(Graph, ParamName);
		break;
	}

	return bChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SViperToolsParameterBehaviorsPanel::OnParamCheckStateChanged(ECheckBoxState NewState, FName ParamName)
{
	UEdGraph* Graph = ResolvedGraph.Get();
	if (!Graph)
	{
		return;
	}

	const bool bFlagged = NewState == ECheckBoxState::Checked;

	switch (CurrentMode)
	{
	case EViperParamBehaviorMode::ExpandAsExecs:
		FViperToolsExpandExecsMetadata::SetParamFlagged(Graph, ParamName, bFlagged);
		break;
	case EViperParamBehaviorMode::OutputTypePicker:
		FViperToolsDynamicOutputMetadata::SetTypePickerParam(Graph, bFlagged ? ParamName : NAME_None);
		break;
	case EViperParamBehaviorMode::DynamicOutputParam:
		FViperToolsDynamicOutputMetadata::SetDynamicOutputParam(Graph, ParamName, bFlagged);
		break;
	case EViperParamBehaviorMode::DefaultToSelf:
		FViperToolsDefaultToSelfMetadata::SetDefaultToSelfParam(Graph, bFlagged ? ParamName : NAME_None);
		break;
	case EViperParamBehaviorMode::AdvancedDisplay:
		FViperToolsAdvancedDisplayMetadata::SetParamFlagged(Graph, ParamName, bFlagged);
		break;
	}
}

FReply SViperToolsParameterBehaviorsPanel::OnApplyClicked()
{
	UBlueprint* Blueprint = ResolvedBlueprint.Get();
	if (!Blueprint)
	{
		StatusText = LOCTEXT("StatusApplyNoBlueprint", "Nothing to apply -- find a function first.");
		return FReply::Handled();
	}

	// Recompiling broadcasts GEditor->OnBlueprintCompiled(), which our module's handler already hooks
	// to re-sync all four behaviors onto the freshly-compiled UFunction.
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	StatusText = FText::Format(LOCTEXT("StatusApplied", "Applied and compiled \"{0}\"."), FText::FromString(Blueprint->GetName()));
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

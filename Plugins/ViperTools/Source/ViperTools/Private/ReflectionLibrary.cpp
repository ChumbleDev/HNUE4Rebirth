// Copyright Weavervilles. All Rights Reserved.

#include "ReflectionLibrary.h"
#include "UObject/UnrealType.h"

namespace
{
	// -----------------------------------------------------------------------
	// Core reflection helpers
	// -----------------------------------------------------------------------

	FProperty* FindProperty(UObject* Target, FName PropertyName)
	{
		if (!Target)
		{
			return nullptr;
		}
		return Target->GetClass()->FindPropertyByName(PropertyName);
	}

	EReflectedPropertyType ClassifyElementType(FProperty* Property)
	{
		if (Property->IsA<FBoolProperty>())      return EReflectedPropertyType::Boolean;
		if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			return ByteProperty->Enum ? EReflectedPropertyType::Enum : EReflectedPropertyType::Byte;
		}
		if (Property->IsA<FEnumProperty>())      return EReflectedPropertyType::Enum;
		if (Property->IsA<FIntProperty>())       return EReflectedPropertyType::Integer;
		if (Property->IsA<FInt64Property>())     return EReflectedPropertyType::Int64;
		if (Property->IsA<FFloatProperty>())     return EReflectedPropertyType::Float;
		if (Property->IsA<FDoubleProperty>())    return EReflectedPropertyType::Double;
		if (Property->IsA<FStrProperty>())       return EReflectedPropertyType::String;
		if (Property->IsA<FNameProperty>())      return EReflectedPropertyType::Name;
		if (Property->IsA<FTextProperty>())      return EReflectedPropertyType::Text;

		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (StructProperty->Struct)
			{
				const FName StructName = StructProperty->Struct->GetFName();
				if (StructName == NAME_Color)       return EReflectedPropertyType::Color;
				if (StructName == NAME_LinearColor) return EReflectedPropertyType::LinearColor;
			}
			return EReflectedPropertyType::Struct;
		}

		if (Property->IsA<FSoftObjectProperty>()) return EReflectedPropertyType::SoftObject;
		if (Property->IsA<FSoftClassProperty>())  return EReflectedPropertyType::SoftClass;
		if (Property->IsA<FClassProperty>())      return EReflectedPropertyType::Class;
		if (Property->IsA<FObjectProperty>())     return EReflectedPropertyType::Object;
		return EReflectedPropertyType::Other;
	}

	void ClassifyProperty(FProperty* Property, EReflectedPropertyType& OutType, EPropertyContainerType& OutContainer)
	{
		if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			OutContainer = EPropertyContainerType::Array;
			OutType = ClassifyElementType(ArrayProperty->Inner);
			return;
		}
		if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
		{
			OutContainer = EPropertyContainerType::Set;
			OutType = ClassifyElementType(SetProperty->ElementProp);
			return;
		}
		if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			OutContainer = EPropertyContainerType::Map;
			OutType = ClassifyElementType(MapProperty->ValueProp); // Type reflects the VALUE, not the key.
			return;
		}
		OutContainer = EPropertyContainerType::Single;
		OutType = ClassifyElementType(Property);
	}

	/** Builds a FPropertyData for Property, whose value lives inside ContainerPtr (an object OR a struct instance). Context is the owning UObject: used for ExportTextItem's owner param and stamped onto Data.Object. */
	FPropertyData BuildPropertyDataFromMemory(FProperty* Property, const void* ContainerPtr, UObject* Context)
	{
		FPropertyData Data;
		Data.Name = Property->GetName();
		Data.Object = Context;

		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(ContainerPtr);
		Property->ExportTextItem(Data.Value, ValuePtr, nullptr, Context, PPF_None);

		ClassifyProperty(Property, Data.Type, Data.Container);
		return Data;
	}

	FPropertyData BuildPropertyData(FProperty* Property, UObject* Target)
	{
		return BuildPropertyDataFromMemory(Property, Target, Target);
	}

	/** Fills OutFields with one PropertyData per field of the struct instance at StructValuePtr. */
	void ExportStructFields(UScriptStruct* ScriptStruct, const void* StructValuePtr, UObject* Context, TArray<FPropertyData>& OutFields)
	{
		OutFields.Empty();

		for (TFieldIterator<FProperty> FieldIt(ScriptStruct); FieldIt; ++FieldIt)
		{
			FProperty* FieldProperty = *FieldIt;
			OutFields.Add(BuildPropertyDataFromMemory(FieldProperty, StructValuePtr, Context));
		}
	}

	// -----------------------------------------------------------------------
	// String parsing helpers shared by both single and array/set extractors
	// -----------------------------------------------------------------------

	FString StripQuotesAndTrim(const FString& In)
	{
		FString Trimmed = In.TrimStartAndEnd();
		Trimmed.TrimQuotesInline();
		return Trimmed;
	}

	bool ParseBool(const FString& In, bool& Out) { Out = StripQuotesAndTrim(In).ToBool(); return true; }
	bool ParseByte(const FString& In, uint8& Out) { Out = static_cast<uint8>(FCString::Atoi(*StripQuotesAndTrim(In))); return true; }
	bool ParseInt(const FString& In, int32& Out) { Out = FCString::Atoi(*StripQuotesAndTrim(In)); return true; }
	bool ParseInt64(const FString& In, int64& Out) { Out = FCString::Atoi64(*StripQuotesAndTrim(In)); return true; }
	bool ParseFloat(const FString& In, float& Out) { Out = FCString::Atof(*StripQuotesAndTrim(In)); return true; }
	bool ParseString(const FString& In, FString& Out) { Out = StripQuotesAndTrim(In); return true; }
	bool ParseName(const FString& In, FName& Out) { Out = FName(*StripQuotesAndTrim(In)); return true; }
	bool ParseText(const FString& In, FText& Out) { Out = FText::FromString(StripQuotesAndTrim(In)); return true; }
	bool ParseVector(const FString& In, FVector& Out) { return Out.InitFromString(In); }
	bool ParseRotator(const FString& In, FRotator& Out) { return Out.InitFromString(In); }
	bool ParseColor(const FString& In, FColor& Out) { return Out.InitFromString(In); }
	bool ParseLinearColor(const FString& In, FLinearColor& Out) { return Out.InitFromString(In); }

	bool ParseObject(const FString& In, UObject*& Out)
	{
		FSoftObjectPath Path(StripQuotesAndTrim(In));
		if (!Path.IsValid())
		{
			Out = nullptr;
			return false;
		}
		Out = Path.TryLoad();
		return Out != nullptr;
	}

	/**
	 * Depth-aware split on top-level commas: respects nested (), so an array of
	 * structs like "((X=1,Y=2,Z=3),(X=4,Y=5,Z=6))" splits into two elements, not six.
	 * Also strips a single pair of enclosing parens from the whole container string first.
	 */
	void SplitContainerElements(const FString& RawValue, TArray<FString>& OutElements)
	{
		OutElements.Empty();

		FString Value = RawValue.TrimStartAndEnd();
		if (Value.Len() >= 2 && Value[0] == TEXT('(') && Value[Value.Len() - 1] == TEXT(')'))
		{
			Value = Value.Mid(1, Value.Len() - 2);
		}

		if (Value.IsEmpty())
		{
			return;
		}

		int32 Depth = 0;
		bool bInQuotes = false;
		int32 SegmentStart = 0;

		for (int32 Index = 0; Index < Value.Len(); ++Index)
		{
			const TCHAR Ch = Value[Index];

			if (Ch == TEXT('"'))
			{
				bInQuotes = !bInQuotes;
			}
			else if (!bInQuotes)
			{
				if (Ch == TEXT('(')) { ++Depth; }
				else if (Ch == TEXT(')')) { --Depth; }
				else if (Ch == TEXT(',') && Depth == 0)
				{
					OutElements.Add(Value.Mid(SegmentStart, Index - SegmentStart).TrimStartAndEnd());
					SegmentStart = Index + 1;
				}
			}
		}
		OutElements.Add(Value.Mid(SegmentStart).TrimStartAndEnd());
	}

	template <typename TElement, typename TParseFunc>
	bool ParseArrayGeneric(const FPropertyData& Data, EReflectedPropertyType ExpectedType, TArray<TElement>& OutValues, TParseFunc ParseFunc)
	{
		OutValues.Empty();

		if (Data.Container != EPropertyContainerType::Array || Data.Type != ExpectedType)
		{
			return false;
		}

		TArray<FString> Elements;
		SplitContainerElements(Data.Value, Elements);

		OutValues.Reserve(Elements.Num());
		for (const FString& ElementStr : Elements)
		{
			TElement Parsed{};
			if (!ParseFunc(ElementStr, Parsed))
			{
				OutValues.Empty();
				return false;
			}
			OutValues.Add(Parsed);
		}
		return true;
	}

	template <typename TElement, typename TParseFunc>
	bool ParseSetGeneric(const FPropertyData& Data, EReflectedPropertyType ExpectedType, TSet<TElement>& OutValues, TParseFunc ParseFunc)
	{
		OutValues.Empty();

		if (Data.Container != EPropertyContainerType::Set || Data.Type != ExpectedType)
		{
			return false;
		}

		TArray<FString> Elements;
		SplitContainerElements(Data.Value, Elements);

		for (const FString& ElementStr : Elements)
		{
			TElement Parsed{};
			if (!ParseFunc(ElementStr, Parsed))
			{
				OutValues.Empty();
				return false;
			}
			OutValues.Add(Parsed);
		}
		return true;
	}
}

// ---------------------------------------------------------------------------
// Core reflection
// ---------------------------------------------------------------------------

void UReflectionLibrary::GetAllProperties(UObject* Target, bool bBlueprintVisibleOnly, TArray<FPropertyData>& Properties)
{
	Properties.Empty();
	if (!Target) return;

	for (TFieldIterator<FProperty> PropIt(Target->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (bBlueprintVisibleOnly && !Property->HasAnyPropertyFlags(CPF_BlueprintVisible))
		{
			continue;
		}
		Properties.Add(BuildPropertyData(Property, Target));
	}
}

bool UReflectionLibrary::GetPropertyByName(UObject* Target, FName PropertyName, bool bBlueprintVisibleOnly, FPropertyData& OutData)
{
	OutData = FPropertyData();

	FProperty* Property = FindProperty(Target, PropertyName);
	if (!Property) return false;

	if (bBlueprintVisibleOnly && !Property->HasAnyPropertyFlags(CPF_BlueprintVisible))
	{
		return false;
	}

	OutData = BuildPropertyData(Property, Target);
	return true;
}

FString UReflectionLibrary::ParsePropertyData(const FPropertyData& Data)
{
	const UEnum* TypeEnum = StaticEnum<EReflectedPropertyType>();
	const UEnum* ContainerEnum = StaticEnum<EPropertyContainerType>();

	const FString TypeString = TypeEnum ? TypeEnum->GetNameStringByValue(static_cast<int64>(Data.Type)) : TEXT("Unknown");
	const FString ContainerString = ContainerEnum ? ContainerEnum->GetNameStringByValue(static_cast<int64>(Data.Container)) : TEXT("Unknown");
	const FString ObjectString = Data.Object ? Data.Object->GetName() : TEXT("None");

	return FString::Printf(TEXT("Name: %s | Value: %s | Type: %s | Container: %s | Object: %s"),
		*Data.Name, *Data.Value, *TypeString, *ContainerString, *ObjectString);
}

// ---------------------------------------------------------------------------
// Struct field breakdown
// ---------------------------------------------------------------------------

bool UReflectionLibrary::GetStructPropertyFields(UObject* Target, FName PropertyName, TArray<FPropertyData>& OutFields)
{
	OutFields.Empty();

	FProperty* Property = FindProperty(Target, PropertyName);
	if (!Property) return false;

	FStructProperty* StructProperty = CastField<FStructProperty>(Property);
	if (!StructProperty) return false;

	const void* StructValuePtr = StructProperty->ContainerPtrToValuePtr<void>(Target);
	ExportStructFields(StructProperty->Struct, StructValuePtr, Target, OutFields);
	return true;
}

bool UReflectionLibrary::GetStructArrayPropertyFields(UObject* Target, FName PropertyName, TArray<FStructFieldsData>& OutElements)
{
	OutElements.Empty();

	FProperty* Property = FindProperty(Target, PropertyName);
	if (!Property) return false;

	FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property);
	if (!ArrayProperty) return false;

	FStructProperty* InnerStructProperty = CastField<FStructProperty>(ArrayProperty->Inner);
	if (!InnerStructProperty) return false;

	const void* ArrayValuePtr = ArrayProperty->ContainerPtrToValuePtr<void>(Target);
	FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayValuePtr);

	for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
	{
		const void* ElementStructPtr = ArrayHelper.GetRawPtr(Index);

		FStructFieldsData ElementFields;
		ExportStructFields(InnerStructProperty->Struct, ElementStructPtr, Target, ElementFields.Fields);

		OutElements.Add(ElementFields);
	}

	return true;
}

// ---------------------------------------------------------------------------
// Single-value extractors
// ---------------------------------------------------------------------------

bool UReflectionLibrary::TryGetAsBool(const FPropertyData& Data, bool& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Boolean) return false;
	return ParseBool(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsByte(const FPropertyData& Data, uint8& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Byte) return false;
	return ParseByte(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsInt(const FPropertyData& Data, int32& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Integer) return false;
	return ParseInt(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsInt64(const FPropertyData& Data, int64& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Int64) return false;
	return ParseInt64(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsFloat(const FPropertyData& Data, float& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Float) return false;
	return ParseFloat(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsString(const FPropertyData& Data, FString& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::String) return false;
	return ParseString(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsName(const FPropertyData& Data, FName& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Name) return false;
	return ParseName(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsText(const FPropertyData& Data, FText& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Text) return false;
	return ParseText(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsVector(const FPropertyData& Data, FVector& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Struct) return false;
	return ParseVector(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsRotator(const FPropertyData& Data, FRotator& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Struct) return false;
	return ParseRotator(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsColor(const FPropertyData& Data, FColor& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Color) return false;
	return ParseColor(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsLinearColor(const FPropertyData& Data, FLinearColor& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::LinearColor) return false;
	return ParseLinearColor(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsObject(const FPropertyData& Data, UObject*& OutValue)
{
	OutValue = nullptr;
	if (Data.Container != EPropertyContainerType::Single) return false;
	if (Data.Type != EReflectedPropertyType::Object && Data.Type != EReflectedPropertyType::SoftObject) return false;
	return ParseObject(Data.Value, OutValue);
}

bool UReflectionLibrary::TryGetAsSoftObjectPath(const FPropertyData& Data, FSoftObjectPath& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::SoftObject) return false;
	OutValue = FSoftObjectPath(StripQuotesAndTrim(Data.Value));
	return OutValue.IsValid();
}

bool UReflectionLibrary::TryGetAsSoftClassPath(const FPropertyData& Data, FSoftClassPath& OutValue)
{
	if (Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::SoftClass) return false;
	OutValue = FSoftClassPath(StripQuotesAndTrim(Data.Value));
	return OutValue.IsValid();
}

bool UReflectionLibrary::TryGetAsEnum(const FPropertyData& Data, UEnum* EnumType, uint8& OutValue)
{
	OutValue = 0;

	if (!EnumType || Data.Container != EPropertyContainerType::Single || Data.Type != EReflectedPropertyType::Enum)
	{
		return false;
	}

	const FString CleanName = StripQuotesAndTrim(Data.Value);
	const int64 EnumValue = EnumType->GetValueByNameString(CleanName);

	if (EnumValue == INDEX_NONE) return false;

	OutValue = static_cast<uint8>(EnumValue);
	return true;
}

// ---------------------------------------------------------------------------
// Array extractors
// ---------------------------------------------------------------------------

bool UReflectionLibrary::TryGetAsBoolArray(const FPropertyData& Data, TArray<bool>& OutValues)
{
	return ParseArrayGeneric<bool>(Data, EReflectedPropertyType::Boolean, OutValues, ParseBool);
}

bool UReflectionLibrary::TryGetAsByteArray(const FPropertyData& Data, TArray<uint8>& OutValues)
{
	return ParseArrayGeneric<uint8>(Data, EReflectedPropertyType::Byte, OutValues, ParseByte);
}

bool UReflectionLibrary::TryGetAsIntArray(const FPropertyData& Data, TArray<int32>& OutValues)
{
	return ParseArrayGeneric<int32>(Data, EReflectedPropertyType::Integer, OutValues, ParseInt);
}

bool UReflectionLibrary::TryGetAsFloatArray(const FPropertyData& Data, TArray<float>& OutValues)
{
	return ParseArrayGeneric<float>(Data, EReflectedPropertyType::Float, OutValues, ParseFloat);
}

bool UReflectionLibrary::TryGetAsStringArray(const FPropertyData& Data, TArray<FString>& OutValues)
{
	return ParseArrayGeneric<FString>(Data, EReflectedPropertyType::String, OutValues, ParseString);
}

bool UReflectionLibrary::TryGetAsNameArray(const FPropertyData& Data, TArray<FName>& OutValues)
{
	return ParseArrayGeneric<FName>(Data, EReflectedPropertyType::Name, OutValues, ParseName);
}

bool UReflectionLibrary::TryGetAsVectorArray(const FPropertyData& Data, TArray<FVector>& OutValues)
{
	return ParseArrayGeneric<FVector>(Data, EReflectedPropertyType::Struct, OutValues, ParseVector);
}

bool UReflectionLibrary::TryGetAsColorArray(const FPropertyData& Data, TArray<FColor>& OutValues)
{
	return ParseArrayGeneric<FColor>(Data, EReflectedPropertyType::Color, OutValues, ParseColor);
}

bool UReflectionLibrary::TryGetAsEnumArray(const FPropertyData& Data, UEnum* EnumType, TArray<uint8>& OutValues)
{
	OutValues.Empty();

	if (!EnumType || Data.Container != EPropertyContainerType::Array || Data.Type != EReflectedPropertyType::Enum)
	{
		return false;
	}

	TArray<FString> Elements;
	SplitContainerElements(Data.Value, Elements);

	OutValues.Reserve(Elements.Num());
	for (const FString& ElementStr : Elements)
	{
		const FString CleanName = StripQuotesAndTrim(ElementStr);
		const int64 EnumValue = EnumType->GetValueByNameString(CleanName);

		if (EnumValue == INDEX_NONE)
		{
			OutValues.Empty();
			return false;
		}

		OutValues.Add(static_cast<uint8>(EnumValue));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Set extractors
// ---------------------------------------------------------------------------

bool UReflectionLibrary::TryGetAsIntSet(const FPropertyData& Data, TSet<int32>& OutValues)
{
	return ParseSetGeneric<int32>(Data, EReflectedPropertyType::Integer, OutValues, ParseInt);
}

bool UReflectionLibrary::TryGetAsFloatSet(const FPropertyData& Data, TSet<float>& OutValues)
{
	return ParseSetGeneric<float>(Data, EReflectedPropertyType::Float, OutValues, ParseFloat);
}

bool UReflectionLibrary::TryGetAsStringSet(const FPropertyData& Data, TSet<FString>& OutValues)
{
	return ParseSetGeneric<FString>(Data, EReflectedPropertyType::String, OutValues, ParseString);
}

bool UReflectionLibrary::TryGetAsNameSet(const FPropertyData& Data, TSet<FName>& OutValues)
{
	return ParseSetGeneric<FName>(Data, EReflectedPropertyType::Name, OutValues, ParseName);
}

// ---------------------------------------------------------------------------
// Map splitter
// ---------------------------------------------------------------------------

bool UReflectionLibrary::TryGetMapAsKeyValueStrings(const FPropertyData& Data, TArray<FString>& OutKeys, TArray<FString>& OutValues)
{
	OutKeys.Empty();
	OutValues.Empty();

	if (Data.Container != EPropertyContainerType::Map)
	{
		return false;
	}

	TArray<FString> Entries;
	SplitContainerElements(Data.Value, Entries);

	OutKeys.Reserve(Entries.Num());
	OutValues.Reserve(Entries.Num());

	for (const FString& Entry : Entries)
	{
		FString KeyPart, ValuePart;
		if (!Entry.Split(TEXT("="), &KeyPart, &ValuePart))
		{
			OutKeys.Empty();
			OutValues.Empty();
			return false;
		}
		OutKeys.Add(StripQuotesAndTrim(KeyPart));
		OutValues.Add(ValuePart.TrimStartAndEnd());
	}

	return true;
}
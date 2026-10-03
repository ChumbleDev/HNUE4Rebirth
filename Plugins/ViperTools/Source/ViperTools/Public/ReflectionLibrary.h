// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReflectionLibrary.generated.h"

UENUM(BlueprintType)
enum class EReflectedPropertyType : uint8
{
	Boolean,
	Byte,
	Integer,
	Int64,
	Float,
	Double,
	String,
	Name,
	Text,
	Enum,
	Struct,
	Color,
	LinearColor,
	Object,
	Class,
	SoftObject,
	SoftClass,
	Other
};

UENUM(BlueprintType)
enum class EPropertyContainerType : uint8
{
	Single,
	Array,
	Set,
	Map
};

/** One reflected property, fully self-describing: what it's called, its stringified value, its element type, and how it's contained (single/array/set/map). */
USTRUCT(BlueprintType)
struct VIPERTOOLS_API FPropertyData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Reflection")
	FString Name;

	UPROPERTY(BlueprintReadWrite, Category = "Reflection")
	FString Value;

	/** The object instance that owns this property (i.e. the Target passed into the reflection call). */
	UPROPERTY(BlueprintReadWrite, Category = "Reflection")
	UObject* Object = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Reflection")
	EReflectedPropertyType Type = EReflectedPropertyType::Other;

	UPROPERTY(BlueprintReadWrite, Category = "Reflection")
	EPropertyContainerType Container = EPropertyContainerType::Single;
};

/** One struct instance broken into its fields (used for array-of-structs; Blueprint can't nest TArray<TArray<FPropertyData>>). */
USTRUCT(BlueprintType)
struct VIPERTOOLS_API FStructFieldsData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Reflection")
	TArray<FPropertyData> Fields;
};

UCLASS()
class VIPERTOOLS_API UReflectionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ---------------------------------------------------------------------
	// Core reflection
	// ---------------------------------------------------------------------

	/** Returns every UPROPERTY on Target as a PropertyData array. If bBlueprintVisibleOnly, only Blueprint-visible properties are included. */
	UFUNCTION(BlueprintCallable, Category = "Reflection")
	static void GetAllProperties(UObject* Target, bool bBlueprintVisibleOnly, TArray<FPropertyData>& Properties);

	/** Returns a single named UPROPERTY on Target as PropertyData. Returns false if not found or not Blueprint-visible (when required). */
	UFUNCTION(BlueprintCallable, Category = "Reflection")
	static bool GetPropertyByName(UObject* Target, FName PropertyName, bool bBlueprintVisibleOnly, FPropertyData& OutData);

	/** Formats a PropertyData as a single human-readable debug string: Name, Value, Type, Container, and the owning Object. */
	UFUNCTION(BlueprintPure, Category = "Reflection")
	static FString ParsePropertyData(const FPropertyData& Data);

	// ---------------------------------------------------------------------
	// Struct field breakdown — no wildcard pins, no CustomThunk. Reflects a struct
	// property's fields directly off Target into an array of PropertyData, which you
	// then run through the typed extractors below and feed into a Make Struct node.
	// ---------------------------------------------------------------------

	/** If PropertyName on Target is a single struct, returns its fields as PropertyData. Returns false if not found or not a single struct. */
	UFUNCTION(BlueprintCallable, Category = "Reflection|Extract Struct")
	static bool GetStructPropertyFields(UObject* Target, FName PropertyName, TArray<FPropertyData>& OutFields);

	/** If PropertyName on Target is an array of structs, returns one field-set per element (index-aligned). Returns false if not found or not an array of structs. */
	UFUNCTION(BlueprintCallable, Category = "Reflection|Extract Struct")
	static bool GetStructArrayPropertyFields(UObject* Target, FName PropertyName, TArray<FStructFieldsData>& OutElements);

	// ---------------------------------------------------------------------
	// Safe typed extractors — single values.
	// Each returns false if Data.Container isn't Single, Data.Type doesn't match, or the string fails to parse.
	// ---------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsBool(const FPropertyData& Data, bool& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsByte(const FPropertyData& Data, uint8& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsInt(const FPropertyData& Data, int32& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsInt64(const FPropertyData& Data, int64& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsFloat(const FPropertyData& Data, float& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsString(const FPropertyData& Data, FString& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsName(const FPropertyData& Data, FName& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsText(const FPropertyData& Data, FText& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsVector(const FPropertyData& Data, FVector& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsRotator(const FPropertyData& Data, FRotator& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsColor(const FPropertyData& Data, FColor& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsLinearColor(const FPropertyData& Data, FLinearColor& OutValue);

	/** Resolves an Object/Class-typed property's stringified path back to a live UObject*. Works for SoftObject too (loads it). */
	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsObject(const FPropertyData& Data, UObject*& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsSoftObjectPath(const FPropertyData& Data, FSoftObjectPath& OutValue);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsSoftClassPath(const FPropertyData& Data, FSoftClassPath& OutValue);

	/** Parses a single Enum-typed PropertyData's stringified enumerator name back to its numeric value, using EnumType. */
	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Single")
	static bool TryGetAsEnum(const FPropertyData& Data, UEnum* EnumType, uint8& OutValue);

	// ---------------------------------------------------------------------
	// Safe typed extractors — Array containers.
	// ---------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Array")
	static bool TryGetAsBoolArray(const FPropertyData& Data, TArray<bool>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Array")
	static bool TryGetAsByteArray(const FPropertyData& Data, TArray<uint8>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Array")
	static bool TryGetAsIntArray(const FPropertyData& Data, TArray<int32>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Array")
	static bool TryGetAsFloatArray(const FPropertyData& Data, TArray<float>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Array")
	static bool TryGetAsStringArray(const FPropertyData& Data, TArray<FString>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Array")
	static bool TryGetAsNameArray(const FPropertyData& Data, TArray<FName>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Array")
	static bool TryGetAsVectorArray(const FPropertyData& Data, TArray<FVector>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Array")
	static bool TryGetAsColorArray(const FPropertyData& Data, TArray<FColor>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Array")
	static bool TryGetAsEnumArray(const FPropertyData& Data, UEnum* EnumType, TArray<uint8>& OutValues);

	// ---------------------------------------------------------------------
	// Safe typed extractors — Set containers.
	// ---------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Set")
	static bool TryGetAsIntSet(const FPropertyData& Data, TSet<int32>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Set")
	static bool TryGetAsFloatSet(const FPropertyData& Data, TSet<float>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Set")
	static bool TryGetAsStringSet(const FPropertyData& Data, TSet<FString>& OutValues);

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Set")
	static bool TryGetAsNameSet(const FPropertyData& Data, TSet<FName>& OutValues);

	// ---------------------------------------------------------------------
	// Map handling — split into parallel key/value string arrays, then run each
	// through the single-type array extractors above.
	// ---------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Reflection|Extract Map")
	static bool TryGetMapAsKeyValueStrings(const FPropertyData& Data, TArray<FString>& OutKeys, TArray<FString>& OutValues);
};

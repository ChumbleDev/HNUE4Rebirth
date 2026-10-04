// Copyright Weavervilles. All Rights Reserved.

#include "ObjectUtilityLibrary.h"
#include "UObject/UObjectHash.h"

TArray<UObject*> UObjectUtilityLibrary::GetInnerObjectsByClass(UObject* Outer, TSubclassOf<UObject> ObjectClass, bool bIncludeNestedSubobjects)
{
	TArray<UObject*> Result;

	if (!Outer)
	{
		return Result;
	}

	const UClass* FilterClass = ObjectClass ? ObjectClass.Get() : UObject::StaticClass();

	TArray<UObject*> InnerObjects;
	GetObjectsWithOuter(Outer, InnerObjects, bIncludeNestedSubobjects);

	Result.Reserve(InnerObjects.Num());
	for (UObject* InnerObject : InnerObjects)
	{
		if (InnerObject && InnerObject->IsA(FilterClass))
		{
			Result.Add(InnerObject);
		}
	}

	return Result;
}

TArray<UObject*> UObjectUtilityLibrary::GetAllActiveObjectsOfClass(TSubclassOf<UObject> ObjectClass)
{
	TArray<UObject*> Result;

	const UClass* FilterClass = ObjectClass ? ObjectClass.Get() : UObject::StaticClass();

	// ExcludeFlags skips class default objects; ExclusionInternalFlags skips objects already marked
	// pending kill/garbage, leaving only genuinely "active" live instances.
	GetObjectsOfClass(FilterClass, Result, /*bIncludeDerivedClasses*/ true, RF_ClassDefaultObject, EInternalObjectFlags::PendingKill);

	return Result;
}

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

// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ObjectUtilityLibrary.generated.h"

/** General-purpose UObject utility functions (not tied to the property-reflection library). */
UCLASS()
class VIPERTOOLS_API UObjectUtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Returns every inner UObject of Outer (i.e. objects whose Outer is Outer) that is of ObjectClass or a subclass of it.
	 * Works just like GetComponentsByClass, but for generic inner/sub-objects instead of actor components — the
	 * returned array's type in Blueprint automatically matches whatever class you plug into ObjectClass.
	 *
	 * @param Outer						The object to search inner objects of.
	 * @param ObjectClass				The class to filter inner objects by. Determines the Blueprint return type.
	 * @param bIncludeNestedSubobjects	If true, also searches inner objects of inner objects (recursively). If false, only direct inner objects are considered.
	 */
	UFUNCTION(BlueprintPure, Category = "Viper Tools|Object Utility", meta = (DeterminesOutputType = "ObjectClass", DefaultToSelf = "Outer"))
	static TArray<UObject*> GetInnerObjectsByClass(UObject* Outer, TSubclassOf<UObject> ObjectClass, bool bIncludeNestedSubobjects = true);
};

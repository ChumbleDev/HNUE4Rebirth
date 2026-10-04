// Copyright Weavervilles. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ObjectUtilityEditorLibrary.generated.h"

/**
 * Editor-only counterpart to the Runtime module's UObjectUtilityLibrary. Lives here (rather than in
 * ObjectUtilityLibrary) because creating a Blueprint asset requires UnrealEd/KismetEditorUtilities,
 * which aren't available in packaged/runtime builds -- putting it in the Runtime module would break
 * packaging. Usable from Editor Utility Blueprints/Widgets and other editor-only Blueprint graphs.
 */
UCLASS()
class VIPERTOOLSEDITOR_API UObjectUtilityEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Creates a new child Blueprint of ParentClass and saves it to disk at AssetPath.
	 *
	 * @param ParentClass		The class the new Blueprint should inherit from. Must be a class Blueprints are allowed to subclass, unless bBypassBlueprintableCheck is true.
	 * @param AssetPath			Full asset path for the new Blueprint, including its name (e.g. "/Game/Blueprints/BP_MyChild"). Fails if an asset already exists there.
	 * @param bBypassBlueprintableCheck	If true, skips the engine's "is this class allowed to be a Blueprint base" check (FKismetEditorUtilities::CanCreateBlueprintOfClass) -- e.g. this lets
	 *									you subclass component types like UBoxComponent that the editor normally won't let you make a Blueprint of directly. Use with care: classes that were
	 *									never designed to be Blueprint bases may compile but behave oddly, fail to instantiate correctly, or be otherwise unsupported. Not recommended for classes
	 *									flagged Deprecated or Abstract -- those are still likely to misbehave or fail regardless of this flag.
	 * @param NewBlueprint		The newly created Blueprint asset, or null on failure.
	 * @return					True if the Blueprint was created and saved successfully.
	 */
	UFUNCTION(BlueprintCallable, Category = "Viper Tools|Object Utility")
	static bool CreateChildBlueprint(TSubclassOf<UObject> ParentClass, const FString& AssetPath, bool bBypassBlueprintableCheck, UBlueprint*& NewBlueprint);

	/**
	 * Returns true if Class is a class the engine normally allows Blueprints to be created from (i.e. what CreateChildBlueprint checks before creating, unless bypassed).
	 * Wraps FKismetEditorUtilities::CanCreateBlueprintOfClass -- e.g. this returns false for component types like UBoxComponent that aren't normally Blueprint-able directly.
	 *
	 * @param Class	The class to check.
	 */
	UFUNCTION(BlueprintCallable, Category = "Viper Tools|Object Utility")
	static bool CanCreateBlueprintOfClass(TSubclassOf<UObject> Class);
};

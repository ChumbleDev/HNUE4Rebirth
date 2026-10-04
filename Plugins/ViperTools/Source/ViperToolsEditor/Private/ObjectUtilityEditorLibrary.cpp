// Copyright Weavervilles. All Rights Reserved.

#include "ObjectUtilityEditorLibrary.h"
#include "ViperToolsEditor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

bool UObjectUtilityEditorLibrary::CreateChildBlueprint(TSubclassOf<UObject> ParentClass, const FString& AssetPath, bool bBypassBlueprintableCheck, UBlueprint*& NewBlueprint)
{
	NewBlueprint = nullptr;

	UClass* ParentClassPtr = ParentClass.Get();
	if (!ParentClassPtr)
	{
		UE_LOG(LogViperTools, Warning, TEXT("CreateChildBlueprint: ParentClass is null."));
		return false;
	}

	if (!bBypassBlueprintableCheck && !FKismetEditorUtilities::CanCreateBlueprintOfClass(ParentClassPtr))
	{
		UE_LOG(LogViperTools, Warning, TEXT("CreateChildBlueprint: Blueprints cannot be created from class '%s' (enable bBypassBlueprintableCheck to override)."), *ParentClassPtr->GetName());
		return false;
	}

	if (bBypassBlueprintableCheck)
	{
		UE_LOG(LogViperTools, Warning, TEXT("CreateChildBlueprint: bBypassBlueprintableCheck is enabled -- skipping the engine's Blueprint-base validation for class '%s'. This class was not necessarily designed to be subclassed via Blueprint; proceed at your own risk."), *ParentClassPtr->GetName());
	}

	FText PathErrorReason;
	if (AssetPath.IsEmpty() || !FPackageName::IsValidLongPackageName(AssetPath, /*bIncludeReadOnlyRoots*/ false, &PathErrorReason))
	{
		UE_LOG(LogViperTools, Warning, TEXT("CreateChildBlueprint: AssetPath '%s' is not a valid asset path. %s"), *AssetPath, *PathErrorReason.ToString());
		return false;
	}

	if (FPackageName::DoesPackageExist(AssetPath))
	{
		UE_LOG(LogViperTools, Warning, TEXT("CreateChildBlueprint: An asset already exists at '%s'. Choose a different path."), *AssetPath);
		return false;
	}

	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		UE_LOG(LogViperTools, Warning, TEXT("CreateChildBlueprint: Failed to create a package at '%s'."), *AssetPath);
		return false;
	}

	const FString AssetName = FPackageName::GetShortName(AssetPath);

	UBlueprint* CreatedBlueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClassPtr,
		Package,
		FName(*AssetName),
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		FName("CreateChildBlueprint"));

	if (!CreatedBlueprint)
	{
		UE_LOG(LogViperTools, Warning, TEXT("CreateChildBlueprint: FKismetEditorUtilities::CreateBlueprint failed for '%s'."), *AssetPath);
		return false;
	}

	FAssetRegistryModule::AssetCreated(CreatedBlueprint);
	Package->MarkPackageDirty();

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());

	const bool bSaved = UPackage::SavePackage(Package, CreatedBlueprint, RF_Public | RF_Standalone, *PackageFileName);

	if (!bSaved)
	{
		UE_LOG(LogViperTools, Warning, TEXT("CreateChildBlueprint: Blueprint was created in memory but failed to save to disk at '%s'."), *PackageFileName);
		return false;
	}

	NewBlueprint = CreatedBlueprint;
	return true;
}

bool UObjectUtilityEditorLibrary::CanCreateBlueprintOfClass(TSubclassOf<UObject> Class)
{
	UClass* ClassPtr = Class.Get();
	return ClassPtr && FKismetEditorUtilities::CanCreateBlueprintOfClass(ClassPtr);
}

// Copyright Weavervilles. All Rights Reserved.

using UnrealBuildTool;

public class ViperToolsEditor : ModuleRules
{
	public ViperToolsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"BlueprintGraph",
			"Kismet",
			"AssetRegistry"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"EditorStyle",
			"PropertyEditor",
			"WorkspaceMenuStructure"
		});
	}
}

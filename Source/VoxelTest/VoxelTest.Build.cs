// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VoxelTest : ModuleRules
{
	public VoxelTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Voxel"
		});

		PrivateIncludePaths.AddRange(new string[] {
			"Voxel/Public"
		});
    }
}

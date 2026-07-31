// Copyright shadowbanefps. Primary runtime module.

using UnrealBuildTool;

public class ShadowbaneFPS : ModuleRules
{
	public ShadowbaneFPS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Flat module layout (Core/, Characters/, …) — needed so includes like
		// "Characters/SBCharacterArchetype.h" resolve under UE 5.5.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"ProceduralMeshComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"NetCore",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"Json",
			"JsonUtilities"
		});
	}
}

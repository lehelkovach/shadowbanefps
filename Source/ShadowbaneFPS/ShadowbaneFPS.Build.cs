// Copyright shadowbanefps. Primary runtime module.

using UnrealBuildTool;

public class ShadowbaneFPS : ModuleRules
{
	public ShadowbaneFPS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"NetCore",
			"OnlineSubsystem",
			"OnlineSubsystemUtils"
		});
	}
}

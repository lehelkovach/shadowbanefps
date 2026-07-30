// Copyright shadowbanefps. Client/standalone build target.

using UnrealBuildTool;
using System.Collections.Generic;

public class ShadowbaneFPSTarget : TargetRules
{
	public ShadowbaneFPSTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("ShadowbaneFPS");
	}
}

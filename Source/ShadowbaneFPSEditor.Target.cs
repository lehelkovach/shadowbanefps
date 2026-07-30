// Copyright shadowbanefps. Editor build target.

using UnrealBuildTool;
using System.Collections.Generic;

public class ShadowbaneFPSEditorTarget : TargetRules
{
	public ShadowbaneFPSEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("ShadowbaneFPS");
	}
}

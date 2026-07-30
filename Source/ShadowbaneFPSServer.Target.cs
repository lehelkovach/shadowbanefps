// Copyright shadowbanefps. Dedicated server build target.
//
// Build the Linux dedicated server (e.g. for an OCI VM) from a Windows/Linux editor host:
//   RunUAT BuildCookRun -project=ShadowbaneFPS.uproject -noP4 \
//     -platform=Linux -serverconfig=Development -server -noclient \
//     -cook -stage -pak -archive -archivedirectory=Dist/Server
//
// See docs/SETUP.md for the full server pipeline.

using UnrealBuildTool;
using System.Collections.Generic;

public class ShadowbaneFPSServerTarget : TargetRules
{
	public ShadowbaneFPSServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("ShadowbaneFPS");
	}
}

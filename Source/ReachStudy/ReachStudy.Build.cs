using UnrealBuildTool;

public class ReachStudy : ModuleRules
{
	public ReachStudy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Meta XR / ISDK modules get added by the spec that first uses them (S1 calibration, S2 contact)
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
	}
}

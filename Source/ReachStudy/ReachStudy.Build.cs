using UnrealBuildTool;

public class ReachStudy : ModuleRules
{
	public ReachStudy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "XRBase",
			"OculusInteraction", "OculusInteractionPrebuilts", "UMG"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "PhysicsCore", "Slate", "SlateCore", "VRCalibrationCore" });
	}
}

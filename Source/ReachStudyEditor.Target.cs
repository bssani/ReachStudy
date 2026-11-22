using UnrealBuildTool;

public class ReachStudyEditorTarget : TargetRules
{
	public ReachStudyEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("ReachStudy");
	}
}

using UnrealBuildTool;
using System.Collections.Generic;

public class SignalLostEditorTarget : TargetRules
{
    public SignalLostEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V4;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.AddRange(new string[] { "SignalLost" });
    }
}

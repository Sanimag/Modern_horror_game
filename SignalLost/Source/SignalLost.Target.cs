using UnrealBuildTool;
using System.Collections.Generic;

public class SignalLostTarget : TargetRules
{
    public SignalLostTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V4;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.AddRange(new string[] { "SignalLost" });
    }
}

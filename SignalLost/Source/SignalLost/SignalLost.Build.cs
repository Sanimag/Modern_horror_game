using UnrealBuildTool;

public class SignalLost : ModuleRules
{
    public SignalLost(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Slate",
            "SlateCore",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "Niagara",
            "NavigationSystem",
            "AIModule",
            "GameplayTasks",
            "ProceduralMeshComponent",
            "PhysicsCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "VoiceChat",
            "AudioMixer",
            "SignalProcessing",
            "Json",
            "JsonUtilities",
            "HTTP"
        });

        PublicIncludePaths.AddRange(new string[]
        {
            "SignalLost/Core",
            "SignalLost/Player",
            "SignalLost/Enemies",
            "SignalLost/Equipment",
            "SignalLost/World",
            "SignalLost/Multiplayer",
            "SignalLost/Progression",
            "SignalLost/UI",
            "SignalLost/Mission"
        });
    }
}

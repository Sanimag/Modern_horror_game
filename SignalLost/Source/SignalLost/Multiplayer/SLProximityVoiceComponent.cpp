#include "SLProximityVoiceComponent.h"
#include "Core/SLGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

USLProximityVoiceComponent::USLProximityVoiceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // Default phantom audio cues
    PhantomAudioCues = {
        FName("Phantom_Footsteps"),
        FName("Phantom_DoorSlam"),
        FName("Phantom_CrewVoice_Help"),
        FName("Phantom_CrewVoice_Behind"),
        FName("Phantom_CrewVoice_RunNow"),
        FName("Phantom_Breathing"),
        FName("Phantom_RadioStatic"),
        FName("Phantom_MetalScrape"),
        FName("Phantom_WaterDrip"),
        FName("Phantom_Whisper")
    };
}

void USLProximityVoiceComponent::BeginPlay()
{
    Super::BeginPlay();
}

void USLProximityVoiceComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Update degradation from game mode
    ASLGameMode* GM = Cast<ASLGameMode>(UGameplayStatics::GetGameMode(GetOwner()));
    if (GM)
    {
        UpdateSignalDegradation(GM->CorruptionIndex);
    }

    TickRadioBattery(DeltaTime);
    TickPhantomAudio(DeltaTime);
}

// ============================================================================
// PROXIMITY VOICE
// ============================================================================

float USLProximityVoiceComponent::GetVolumeForDistance(float Distance) const
{
    if (Distance >= CurrentProximityRange) return 0.0f;
    if (Distance <= 0.0f) return 1.0f;

    // Inverse square falloff
    float NormalizedDist = Distance / CurrentProximityRange;
    return FMath::Clamp(1.0f - (NormalizedDist * NormalizedDist), 0.0f, 1.0f);
}

float USLProximityVoiceComponent::GetVoiceDistortionLevel() const
{
    // At 0% corruption: no distortion
    // At 50%: slight static
    // At 75%+: heavy distortion, voice cutting
    return DegradationLevel;
}

// ============================================================================
// RADIO
// ============================================================================

void USLProximityVoiceComponent::ToggleRadio()
{
    if (RadioBatteryLife <= 0.0f)
    {
        bRadioActive = false;
        return;
    }
    bRadioActive = !bRadioActive;
}

bool USLProximityVoiceComponent::IsRadioUsable() const
{
    return bRadioActive && RadioBatteryLife > 0.0f && RadioReliability > 0.1f;
}

void USLProximityVoiceComponent::TickRadioBattery(float DeltaTime)
{
    if (!bRadioActive) return;

    RadioBatteryLife -= (RadioBatteryDrainRate / 60.0f) * DeltaTime;

    if (RadioBatteryLife <= 0.0f)
    {
        RadioBatteryLife = 0.0f;
        bRadioActive = false;
        UE_LOG(LogSignalLost, Warning, TEXT("Radio battery depleted"));
    }

    // Radio noise output increases with corruption
    RadioNoiseOutput = FMath::Lerp(0.1f, 0.8f, DegradationLevel);
}

// ============================================================================
// SIGNAL DEGRADATION
// ============================================================================

void USLProximityVoiceComponent::UpdateSignalDegradation(float CorruptionPercent)
{
    // Voice range decreases with corruption
    CurrentProximityRange = BaseProximityRange *
        FMath::Lerp(1.0f, 0.3f, CorruptionPercent / 100.0f);

    // Radio reliability decreases
    if (CorruptionPercent < 25.0f)
    {
        RadioReliability = 1.0f;
        DegradationLevel = 0.0f;
    }
    else
    {
        RadioReliability = FMath::Lerp(1.0f, 0.0f, (CorruptionPercent - 25.0f) / 75.0f);
        DegradationLevel = FMath::Lerp(0.0f, 1.0f, (CorruptionPercent - 25.0f) / 75.0f);
    }

    // Phantom audio interval decreases at high corruption
    PhantomAudioInterval = FMath::Lerp(60.0f, 8.0f, DegradationLevel);
}

// ============================================================================
// PHANTOM AUDIO
// ============================================================================

bool USLProximityVoiceComponent::ShouldInjectPhantomAudio() const
{
    return DegradationLevel > 0.6f; // Only at 75%+ corruption
}

FName USLProximityVoiceComponent::GetRandomPhantomEvent() const
{
    if (PhantomAudioCues.Num() == 0) return NAME_None;
    return PhantomAudioCues[FMath::RandRange(0, PhantomAudioCues.Num() - 1)];
}

void USLProximityVoiceComponent::TickPhantomAudio(float DeltaTime)
{
    if (!ShouldInjectPhantomAudio()) return;

    PhantomAudioTimer += DeltaTime;

    if (PhantomAudioTimer >= PhantomAudioInterval)
    {
        PhantomAudioTimer = 0.0f;

        FName Event = GetRandomPhantomEvent();
        if (!Event.IsNone())
        {
            UE_LOG(LogSignalLost, Log, TEXT("Phantom audio event: %s"), *Event.ToString());
            // Play the phantom audio cue through the voice chat system
            // This makes it indistinguishable from real player communication
        }
    }
}

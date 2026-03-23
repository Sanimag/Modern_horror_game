#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/SLGameTypes.h"
#include "SLProximityVoiceComponent.generated.h"

/**
 * Proximity Voice Chat Component
 * Handles distance-based voice attenuation, radio communication,
 * signal degradation effects, and phantom audio injection.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SIGNALLOST_API USLProximityVoiceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USLProximityVoiceComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // ========================================================================
    // PROXIMITY VOICE
    // ========================================================================

    /** Base range for proximity voice (Unreal units) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice")
    float BaseProximityRange = 2000.0f;

    /** Current effective range (reduced by corruption) */
    UPROPERTY(BlueprintReadOnly, Category = "Voice")
    float CurrentProximityRange = 2000.0f;

    /** Get voice volume multiplier for a listener at given distance */
    UFUNCTION(BlueprintPure, Category = "Voice")
    float GetVolumeForDistance(float Distance) const;

    /** Get static/distortion level for voice processing */
    UFUNCTION(BlueprintPure, Category = "Voice")
    float GetVoiceDistortionLevel() const;

    // ========================================================================
    // RADIO
    // ========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Radio")
    bool bRadioActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio")
    float RadioBatteryLife = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radio")
    float RadioBatteryDrainRate = 5.0f; // % per minute

    /** Radio range (unlimited but affected by reliability) */
    UPROPERTY(BlueprintReadOnly, Category = "Radio")
    float RadioReliability = 1.0f;

    /** Does radio produce audible static that entities can hear? */
    UPROPERTY(BlueprintReadOnly, Category = "Radio")
    float RadioNoiseOutput = 0.0f;

    UFUNCTION(BlueprintCallable, Category = "Radio")
    void ToggleRadio();

    UFUNCTION(BlueprintCallable, Category = "Radio")
    bool IsRadioUsable() const;

    // ========================================================================
    // PHANTOM AUDIO
    // ========================================================================

    /** Should phantom audio be injected into this player's stream? */
    UFUNCTION(BlueprintPure, Category = "Phantom")
    bool ShouldInjectPhantomAudio() const;

    /** Types of phantom audio events */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phantom")
    TArray<FName> PhantomAudioCues;

    /** Phantom footsteps, door slams, voice clips from "previous crews" */
    UFUNCTION(BlueprintCallable, Category = "Phantom")
    FName GetRandomPhantomEvent() const;

    // ========================================================================
    // SIGNAL DEGRADATION
    // ========================================================================

    /** Apply corruption-based degradation to voice systems */
    UFUNCTION(BlueprintCallable, Category = "Signal")
    void UpdateSignalDegradation(float CorruptionPercent);

    /** Current degradation level (0 = clear, 1 = completely degraded) */
    UPROPERTY(BlueprintReadOnly, Category = "Signal")
    float DegradationLevel = 0.0f;

protected:
    virtual void BeginPlay() override;

private:
    void TickRadioBattery(float DeltaTime);
    void TickPhantomAudio(float DeltaTime);

    float PhantomAudioTimer = 0.0f;
    float PhantomAudioInterval = 30.0f; // Base interval between phantom events
};

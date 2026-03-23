#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Core/SLGameTypes.h"
#include "SLHUD.generated.h"

class ASLPlayerCharacter;

/**
 * Minimal HUD for Signal Lost.
 * Design philosophy: minimal on-screen elements.
 * - Health shown through screen vignetting and heartbeat audio
 * - Corruption % only visible to Scanner role
 * - Equipment status shown on physical models (battery lights)
 * - Inventory via wrist-mounted device (player must look at it)
 */
UCLASS()
class SIGNALLOST_API ASLHUD : public AHUD
{
    GENERATED_BODY()

public:
    ASLHUD();

    virtual void DrawHUD() override;

    // ========================================================================
    // HUD ELEMENTS
    // ========================================================================

    /** Draw health vignette (darkening screen edges as health drops) */
    void DrawHealthVignette(float Health, float MaxHealth);

    /** Draw Corruption % (Scanner role only) */
    void DrawCorruptionIndicator(float Corruption, ECorruptionPhase Phase);

    /** Draw Signal Fade timer when downed */
    void DrawSignalFadeTimer(float TimeRemaining);

    /** Draw Cascade countdown */
    void DrawCascadeCountdown(float TimeRemaining);

    /** Draw extraction progress bar */
    void DrawExtractionProgress(float Progress, int32 Phase);

    /** Draw Resync Kit progress */
    void DrawResyncProgress(float Progress);

    /** Draw compass pointing toward airlock */
    void DrawAirlockCompass(FVector AirlockDirection);

    /** Draw teammate indicators (name, distance, role) */
    void DrawTeammateIndicators();

    /** Draw Perception Drift visual effects */
    void DrawPerceptionDriftEffects(float DriftLevel);

    // ========================================================================
    // POST-PROCESS EFFECTS
    // ========================================================================

    /** Frequency Burn screen effect (from The Resonant) */
    UFUNCTION(BlueprintCallable, Category = "Effects")
    void ApplyFrequencyBurnEffect(float Intensity, float Duration);

    /** Broadcast visual distortion effect */
    UFUNCTION(BlueprintCallable, Category = "Effects")
    void ApplyBroadcastDistortion(float Intensity);

    /** Corruption phase ambient effect (color shifts, static) */
    UFUNCTION(BlueprintCallable, Category = "Effects")
    void ApplyCorruptionAmbientEffect(ECorruptionPhase Phase);

    // ========================================================================
    // WRIST DEVICE
    // ========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "WristDevice")
    bool bWristDeviceOpen = false;

    UFUNCTION(BlueprintCallable, Category = "WristDevice")
    void ToggleWristDevice();

    /** Draw the wrist-mounted inventory/status device */
    void DrawWristDevice();

protected:
    UPROPERTY()
    ASLPlayerCharacter* OwnerCharacter = nullptr;

    // Effect timers
    float FrequencyBurnTimer = 0.0f;
    float FrequencyBurnIntensity = 0.0f;
    float BroadcastDistortionIntensity = 0.0f;

    // Perception drift rendering
    float DriftShadowOffset = 0.0f;
    float DriftColorShift = 0.0f;
    bool bDriftPhantomEntity = false;
};

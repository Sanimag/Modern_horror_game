#pragma once

#include "SLEntityBase.h"
#include "SLEntityResonant.generated.h"

/**
 * THE RESONANT (Sound-Based Hunter)
 * Floating mass of translucent geometric shards that hum at low frequency.
 * Blind but has perfect sound tracking. Patrols corridors slowly.
 * Any noise attracts it — footsteps, radio, equipment, even voice chat volume.
 * Counter: Crouch-walk, noise makers, Operator drone distraction.
 */
UCLASS()
class SIGNALLOST_API ASLEntityResonant : public ASLEntityBase
{
    GENERATED_BODY()

public:
    ASLEntityResonant();

    /** Sound detection range */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonant")
    float SoundDetectionRange = 3000.0f;

    /** Minimum noise level to detect (0-1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonant")
    float NoiseThreshold = 0.2f;

    /** Speed when chasing a sound source */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonant")
    float ChaseSpeed = 700.0f;

    /** Speed when patrolling */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonant")
    float PatrolSpeed = 100.0f;

    /** Frequency Burn: damage over time on contact */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonant")
    float FrequencyBurnDPS = 15.0f;

    /** How long it remembers a sound source */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resonant")
    float SoundMemoryDuration = 8.0f;

    /** Current location it's investigating */
    UPROPERTY(BlueprintReadOnly, Category = "Resonant")
    FVector LastHeardLocation = FVector::ZeroVector;

protected:
    virtual void UpdateBehavior(float DeltaTime) override;
    virtual ASLPlayerCharacter* FindTarget() override;

private:
    void Patrol(float DeltaTime);
    void ChaseSound(float DeltaTime);
    void ScanForSounds();

    enum class EResonantState : uint8
    {
        Patrolling,
        Investigating,
        Chasing
    };

    EResonantState ResonantState = EResonantState::Patrolling;
    float SoundMemoryTimer = 0.0f;
    TArray<FVector> PatrolPath;
    int32 CurrentPatrolIndex = 0;
};

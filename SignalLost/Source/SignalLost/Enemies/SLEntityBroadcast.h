#pragma once

#include "SLEntityBase.h"
#include "SLEntityBroadcast.generated.h"

/**
 * THE BROADCAST (Boss Entity)
 * Massive entity of cascading signal waves and fractured light.
 * Fills entire corridors. Looking at it causes screen distortion.
 * Spawns only during Cascade (100% Corruption).
 * Moves slowly but inevitably toward the airlock.
 * Contact is instantly lethal. Jams all equipment within 30m.
 * You don't fight it. You outrun it.
 */
UCLASS()
class SIGNALLOST_API ASLEntityBroadcast : public ASLEntityBase
{
    GENERATED_BODY()

public:
    ASLEntityBroadcast();

    /** Speed toward the airlock */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Broadcast")
    float AdvanceSpeed = 200.0f;

    /** Equipment jamming radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Broadcast")
    float JamRadius = 3000.0f; // 30 meters

    /** Visual distortion radius when looking at it */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Broadcast")
    float DistortionRadius = 5000.0f;

    /** The Broadcast fills corridors — its collision is massive */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Broadcast")
    float KillRadius = 500.0f;

    /** Target location (the airlock) */
    UPROPERTY(BlueprintReadWrite, Category = "Broadcast")
    FVector AirlockTarget = FVector::ZeroVector;

    /** Check if a location is within the equipment jam zone */
    UFUNCTION(BlueprintPure, Category = "Broadcast")
    bool IsLocationJammed(FVector Location) const;

    /** Get visual distortion intensity for a player based on look angle and distance */
    UFUNCTION(BlueprintPure, Category = "Broadcast")
    float GetDistortionIntensity(ASLPlayerCharacter* Player) const;

protected:
    virtual void UpdateBehavior(float DeltaTime) override;

private:
    void AdvanceTowardAirlock(float DeltaTime);
    void KillPlayersInPath();
    void JamNearbyEquipment();
    void ApplyVisualDistortion();
};

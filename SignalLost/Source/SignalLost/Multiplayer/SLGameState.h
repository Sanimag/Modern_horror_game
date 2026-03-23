#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/SLGameTypes.h"
#include "SLGameState.generated.h"

/**
 * Replicated game state for Signal Lost.
 * Tracks mission progress, player states, and shared session data.
 */
UCLASS()
class SIGNALLOST_API ASLGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ASLGameState();

    // ========================================================================
    // MISSION STATE
    // ========================================================================

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission")
    FMissionContract ActiveContract;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission")
    bool bInMission = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission")
    float MissionTimer = 0.0f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission")
    int32 TotalCoresExtracted = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission")
    int32 TotalCreditsEarned = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission")
    float CorruptionIndex = 0.0f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission")
    ECorruptionPhase CurrentPhase = ECorruptionPhase::Quiet;

    // ========================================================================
    // PLAYER TRACKING
    // ========================================================================

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Players")
    int32 AlivePlayerCount = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Players")
    int32 TotalPlayerCount = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Players")
    int32 PlayersAtAirlock = 0;

    // ========================================================================
    // ENTITY TRACKING
    // ========================================================================

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Entities")
    int32 ActiveEntityCount = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Entities")
    bool bBroadcastSpawned = false;

    // ========================================================================
    // CASCADE
    // ========================================================================

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Cascade")
    bool bCascadeActive = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Cascade")
    float CascadeCountdown = 90.0f;

    // ========================================================================
    // WEEKLY CONTRACT
    // ========================================================================

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weekly")
    int32 WeeklyContractSeed = 0;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weekly")
    FText WeeklyContractName;

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

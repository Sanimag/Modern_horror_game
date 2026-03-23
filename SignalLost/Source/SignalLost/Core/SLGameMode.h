#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SLGameTypes.h"
#include "SLGameMode.generated.h"

class ASLPlayerCharacter;
class ASLEntityBase;
class ASLStationGenerator;

/**
 * Main game mode for Signal Lost.
 * Manages the mission lifecycle, corruption index, entity spawning,
 * and coordinates all core systems during a run.
 */
UCLASS()
class SIGNALLOST_API ASLGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASLGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ========================================================================
    // CORRUPTION INDEX SYSTEM
    // ========================================================================

    /** Current station-wide corruption percentage (0-100) */
    UPROPERTY(ReplicatedUsing=OnRep_CorruptionIndex, BlueprintReadOnly, Category = "Corruption")
    float CorruptionIndex = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Corruption")
    ECorruptionPhase CurrentPhase = ECorruptionPhase::Quiet;

    /** Add corruption from core extraction or events */
    UFUNCTION(BlueprintCallable, Category = "Corruption")
    void AddCorruption(float Amount);

    /** Get current corruption phase based on percentage */
    UFUNCTION(BlueprintPure, Category = "Corruption")
    ECorruptionPhase GetCorruptionPhase() const;

    UPROPERTY(BlueprintAssignable, Category = "Corruption")
    FOnCorruptionChanged OnCorruptionChanged;

    UPROPERTY(BlueprintAssignable, Category = "Corruption")
    FOnCascadeTriggered OnCascadeTriggered;

    // ========================================================================
    // ENTITY MANAGEMENT
    // ========================================================================

    /** Spawn an entity of the given type at a valid spawn point */
    UFUNCTION(BlueprintCallable, Category = "Entities")
    ASLEntityBase* SpawnEntity(EEntityType Type, FVector Location);

    /** Get all currently active entities */
    UFUNCTION(BlueprintPure, Category = "Entities")
    TArray<ASLEntityBase*> GetActiveEntities() const { return ActiveEntities; }

    UPROPERTY(BlueprintAssignable, Category = "Entities")
    FOnEntitySpawned OnEntitySpawned;

    // ========================================================================
    // MISSION MANAGEMENT
    // ========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
    FMissionContract CurrentContract;

    UFUNCTION(BlueprintCallable, Category = "Mission")
    void StartMission(const FMissionContract& Contract);

    UFUNCTION(BlueprintCallable, Category = "Mission")
    void InitiateExtraction();

    UFUNCTION(BlueprintCallable, Category = "Mission")
    void CompleteMission();

    UFUNCTION(BlueprintCallable, Category = "Mission")
    void FailMission();

    UPROPERTY(BlueprintReadOnly, Category = "Mission")
    bool bMissionActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Mission")
    bool bCascadeActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Mission")
    float CascadeCountdown = 90.0f;

    // ========================================================================
    // PLAYER MANAGEMENT
    // ========================================================================

    UFUNCTION(BlueprintPure, Category = "Players")
    TArray<ASLPlayerCharacter*> GetAlivePlayers() const;

    UFUNCTION(BlueprintPure, Category = "Players")
    int32 GetPlayerCount() const;

    UFUNCTION(BlueprintCallable, Category = "Players")
    bool AreAllPlayersAtAirlock() const;

    UFUNCTION(BlueprintCallable, Category = "Players")
    bool AreAllPlayersAccountedFor() const;

    UPROPERTY(BlueprintAssignable, Category = "Players")
    FOnPlayerStateChanged OnPlayerStateChanged;

    // ========================================================================
    // SIGNAL DEGRADATION
    // ========================================================================

    /** Get current voice chat range based on corruption */
    UFUNCTION(BlueprintPure, Category = "Signal")
    float GetProximityVoiceRange() const;

    /** Get radio reliability (0-1, where 1 is perfect) */
    UFUNCTION(BlueprintPure, Category = "Signal")
    float GetRadioReliability() const;

    /** Should phantom audio play? */
    UFUNCTION(BlueprintPure, Category = "Signal")
    bool ShouldPlayPhantomAudio() const;

    // ========================================================================
    // STATION
    // ========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Station")
    ASLStationGenerator* StationGenerator = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Station")
    FVector AirlockLocation = FVector::ZeroVector;

protected:
    UFUNCTION()
    void OnRep_CorruptionIndex();

    void UpdateCorruptionPhase();
    void TickCorruptionEffects(float DeltaTime);
    void TickEntitySpawning(float DeltaTime);
    void TickCascade(float DeltaTime);
    void TickSignalDegradation(float DeltaTime);
    void TriggerCascade();

    // Entity spawning timers
    float EntitySpawnTimer = 0.0f;
    float EntitySpawnInterval = 30.0f; // seconds between entity spawn checks

    // Phase transition tracking
    ECorruptionPhase PreviousPhase = ECorruptionPhase::Quiet;

    // Active entities in the station
    UPROPERTY()
    TArray<ASLEntityBase*> ActiveEntities;

    // Max entities per corruption phase
    TMap<ECorruptionPhase, int32> MaxEntitiesPerPhase;

    // Light failure tracking
    float LightFailureTimer = 0.0f;
    int32 FailedLightCount = 0;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

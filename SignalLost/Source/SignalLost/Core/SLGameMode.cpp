#include "SLGameMode.h"
#include "SLGameTypes.h"
#include "Player/SLPlayerCharacter.h"
#include "Enemies/SLEntityBase.h"
#include "World/SLStationGenerator.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

ASLGameMode::ASLGameMode()
{
    PrimaryActorTick.bCanEverTick = true;

    // Entity limits per corruption phase
    MaxEntitiesPerPhase.Add(ECorruptionPhase::Quiet, 0);
    MaxEntitiesPerPhase.Add(ECorruptionPhase::Stirring, 2);
    MaxEntitiesPerPhase.Add(ECorruptionPhase::Active, 5);
    MaxEntitiesPerPhase.Add(ECorruptionPhase::Critical, 8);
    MaxEntitiesPerPhase.Add(ECorruptionPhase::Cascade, 12);
}

void ASLGameMode::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogSignalLost, Log, TEXT("Signal Lost GameMode initialized"));
}

void ASLGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bMissionActive) return;

    TickCorruptionEffects(DeltaTime);
    TickEntitySpawning(DeltaTime);
    TickSignalDegradation(DeltaTime);

    if (bCascadeActive)
    {
        TickCascade(DeltaTime);
    }
}

// ============================================================================
// CORRUPTION INDEX
// ============================================================================

void ASLGameMode::AddCorruption(float Amount)
{
    float PreviousCorruption = CorruptionIndex;
    CorruptionIndex = FMath::Clamp(CorruptionIndex + Amount, 0.0f, 100.0f);

    UpdateCorruptionPhase();

    if (CorruptionIndex >= 100.0f && PreviousCorruption < 100.0f)
    {
        TriggerCascade();
    }

    UE_LOG(LogSignalLost, Log, TEXT("Corruption: %.1f%% -> %.1f%% (Phase: %d)"),
        PreviousCorruption, CorruptionIndex, (int32)CurrentPhase);
}

ECorruptionPhase ASLGameMode::GetCorruptionPhase() const
{
    if (CorruptionIndex >= 100.0f) return ECorruptionPhase::Cascade;
    if (CorruptionIndex >= 75.0f) return ECorruptionPhase::Critical;
    if (CorruptionIndex >= 50.0f) return ECorruptionPhase::Active;
    if (CorruptionIndex >= 25.0f) return ECorruptionPhase::Stirring;
    return ECorruptionPhase::Quiet;
}

void ASLGameMode::UpdateCorruptionPhase()
{
    ECorruptionPhase NewPhase = GetCorruptionPhase();
    if (NewPhase != CurrentPhase)
    {
        PreviousPhase = CurrentPhase;
        CurrentPhase = NewPhase;
        OnCorruptionChanged.Broadcast(CorruptionIndex, CurrentPhase);

        UE_LOG(LogSignalLost, Warning, TEXT("CORRUPTION PHASE CHANGE: %d -> %d"),
            (int32)PreviousPhase, (int32)CurrentPhase);
    }
}

void ASLGameMode::OnRep_CorruptionIndex()
{
    UpdateCorruptionPhase();
}

void ASLGameMode::TickCorruptionEffects(float DeltaTime)
{
    // Systematic light failure at 25%+
    if (CurrentPhase >= ECorruptionPhase::Stirring)
    {
        LightFailureTimer += DeltaTime;
        float FailureInterval = FMath::Lerp(15.0f, 3.0f,
            (CorruptionIndex - 25.0f) / 75.0f);

        if (LightFailureTimer >= FailureInterval)
        {
            LightFailureTimer = 0.0f;
            FailedLightCount++;
            // Signal station generator to fail a light
            if (StationGenerator)
            {
                // StationGenerator->FailRandomLight();
            }
        }
    }
}

// ============================================================================
// ENTITY SPAWNING
// ============================================================================

void ASLGameMode::TickEntitySpawning(float DeltaTime)
{
    if (CurrentPhase < ECorruptionPhase::Active) return;

    EntitySpawnTimer += DeltaTime;

    // Spawn interval decreases with corruption
    float AdjustedInterval = EntitySpawnInterval *
        FMath::Lerp(1.0f, 0.3f, (CorruptionIndex - 50.0f) / 50.0f);

    if (EntitySpawnTimer < AdjustedInterval) return;
    EntitySpawnTimer = 0.0f;

    int32 MaxEntities = MaxEntitiesPerPhase.FindRef(CurrentPhase);
    if (ActiveEntities.Num() >= MaxEntities) return;

    // Select entity type based on corruption level
    TArray<EEntityType> ValidTypes;
    ValidTypes.Add(EEntityType::Echo);      // Always available in Active+
    ValidTypes.Add(EEntityType::Resonant);  // Always available in Active+

    if (CorruptionIndex >= 50.0f)
    {
        ValidTypes.Add(EEntityType::Architect);
        ValidTypes.Add(EEntityType::Mimic);
    }

    if (bCascadeActive)
    {
        ValidTypes.Add(EEntityType::Broadcast);
    }

    EEntityType SelectedType = ValidTypes[FMath::RandRange(0, ValidTypes.Num() - 1)];

    // Find a spawn point away from players
    FVector SpawnLocation = FVector::ZeroVector; // TODO: Get from station generator
    SpawnEntity(SelectedType, SpawnLocation);
}

ASLEntityBase* ASLGameMode::SpawnEntity(EEntityType Type, FVector Location)
{
    // Entity class mapping would be configured via data assets
    UE_LOG(LogSignalLost, Log, TEXT("Spawning entity type %d at %s"),
        (int32)Type, *Location.ToString());

    OnEntitySpawned.Broadcast(Type);

    // Actual spawning handled via Blueprint subclass mapping
    return nullptr;
}

// ============================================================================
// CASCADE
// ============================================================================

void ASLGameMode::TriggerCascade()
{
    bCascadeActive = true;
    CascadeCountdown = 90.0f;

    OnCascadeTriggered.Broadcast(CascadeCountdown);

    UE_LOG(LogSignalLost, Error, TEXT("CASCADE TRIGGERED - 90 seconds to extraction!"));

    // Spawn The Broadcast
    SpawnEntity(EEntityType::Broadcast, FVector::ZeroVector);
}

void ASLGameMode::TickCascade(float DeltaTime)
{
    CascadeCountdown -= DeltaTime;

    if (CascadeCountdown <= 0.0f)
    {
        // Total station failure - all remaining players die
        FailMission();
    }
}

// ============================================================================
// SIGNAL DEGRADATION
// ============================================================================

float ASLGameMode::GetProximityVoiceRange() const
{
    // Full range at 0% corruption, halved at 100%
    float BaseRange = 2000.0f; // Unreal units
    return BaseRange * FMath::Lerp(1.0f, 0.3f, CorruptionIndex / 100.0f);
}

float ASLGameMode::GetRadioReliability() const
{
    if (CorruptionIndex < 25.0f) return 1.0f;
    return FMath::Lerp(1.0f, 0.0f, (CorruptionIndex - 25.0f) / 75.0f);
}

bool ASLGameMode::ShouldPlayPhantomAudio() const
{
    if (CorruptionIndex < 75.0f) return false;
    // 30% chance per check at Critical, always during Cascade
    return bCascadeActive || (FMath::FRand() < 0.3f);
}

void ASLGameMode::TickSignalDegradation(float DeltaTime)
{
    // Radio static intensity increases with corruption
    // Phantom audio injection at Critical+
    // Voice distortion processing updates
}

// ============================================================================
// MISSION MANAGEMENT
// ============================================================================

void ASLGameMode::StartMission(const FMissionContract& Contract)
{
    CurrentContract = Contract;
    bMissionActive = true;
    bCascadeActive = false;
    CorruptionIndex = Contract.BaseCorruption;
    CascadeCountdown = 90.0f;
    EntitySpawnTimer = 0.0f;
    FailedLightCount = 0;
    ActiveEntities.Empty();

    UpdateCorruptionPhase();

    UE_LOG(LogSignalLost, Log, TEXT("Mission started: Threat %d, Payout %.1fx"),
        Contract.ThreatRating, Contract.PayoutMultiplier);
}

void ASLGameMode::InitiateExtraction()
{
    if (!AreAllPlayersAccountedFor())
    {
        UE_LOG(LogSignalLost, Warning, TEXT("Cannot extract - players unaccounted for"));
        return;
    }

    CompleteMission();
}

void ASLGameMode::CompleteMission()
{
    bMissionActive = false;
    bCascadeActive = false;

    UE_LOG(LogSignalLost, Log, TEXT("Mission complete! Final corruption: %.1f%%"),
        CorruptionIndex);
}

void ASLGameMode::FailMission()
{
    bMissionActive = false;
    bCascadeActive = false;

    UE_LOG(LogSignalLost, Error, TEXT("Mission FAILED at corruption: %.1f%%"),
        CorruptionIndex);
}

// ============================================================================
// PLAYER MANAGEMENT
// ============================================================================

TArray<ASLPlayerCharacter*> ASLGameMode::GetAlivePlayers() const
{
    TArray<ASLPlayerCharacter*> AlivePlayers;
    // Iterate registered players and filter by state
    return AlivePlayers;
}

int32 ASLGameMode::GetPlayerCount() const
{
    return GetNumPlayers();
}

bool ASLGameMode::AreAllPlayersAtAirlock() const
{
    // Check all alive players are within airlock radius
    return false;
}

bool ASLGameMode::AreAllPlayersAccountedFor() const
{
    // All players either at airlock or confirmed dead
    return false;
}

void ASLGameMode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASLGameMode, CorruptionIndex);
}

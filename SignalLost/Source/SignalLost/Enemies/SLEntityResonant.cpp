#include "SLEntityResonant.h"
#include "Player/SLPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

ASLEntityResonant::ASLEntityResonant()
{
    EntityType = EEntityType::Resonant;
    AttackDamage = 0.0f; // Uses Frequency Burn DPS instead
    MovementSpeed = PatrolSpeed;
    DetectionRange = SoundDetectionRange;
    PerceptionDriftInflicted = 10.0f;
    bCanBeStunned = true;
}

void ASLEntityResonant::UpdateBehavior(float DeltaTime)
{
    ScanForSounds();

    switch (ResonantState)
    {
    case EResonantState::Patrolling:
        Patrol(DeltaTime);
        break;

    case EResonantState::Investigating:
    case EResonantState::Chasing:
        ChaseSound(DeltaTime);

        SoundMemoryTimer -= DeltaTime;
        if (SoundMemoryTimer <= 0.0f)
        {
            ResonantState = EResonantState::Patrolling;
            CurrentTarget = nullptr;
        }
        break;
    }

    // Frequency Burn: damage players on contact
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ASLPlayerCharacter* Player = Cast<ASLPlayerCharacter>((*It)->GetPawn());
        if (!Player || Player->CurrentState != EPlayerState::Alive) continue;

        float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
        if (Distance < 200.0f) // Contact range
        {
            Player->TakeDamageCustom(FrequencyBurnDPS * DeltaTime, EntityType);
        }
    }
}

void ASLEntityResonant::ScanForSounds()
{
    float LoudestNoise = 0.0f;
    ASLPlayerCharacter* LoudestPlayer = nullptr;
    FVector LoudestLocation = FVector::ZeroVector;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ASLPlayerCharacter* Player = Cast<ASLPlayerCharacter>((*It)->GetPawn());
        if (!Player || Player->CurrentState != EPlayerState::Alive) continue;

        float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
        if (Distance > SoundDetectionRange) continue;

        float Noise = Player->GetNoiseLevel();

        // Distance attenuation
        float AttenuatedNoise = Noise * (1.0f - Distance / SoundDetectionRange);

        if (AttenuatedNoise > NoiseThreshold && AttenuatedNoise > LoudestNoise)
        {
            LoudestNoise = AttenuatedNoise;
            LoudestPlayer = Player;
            LoudestLocation = Player->GetActorLocation();
        }
    }

    if (LoudestPlayer)
    {
        LastHeardLocation = LoudestLocation;
        SoundMemoryTimer = SoundMemoryDuration;
        CurrentTarget = LoudestPlayer;

        if (LoudestNoise > 0.6f)
        {
            ResonantState = EResonantState::Chasing;
            MovementSpeed = ChaseSpeed;
        }
        else
        {
            ResonantState = EResonantState::Investigating;
            MovementSpeed = ChaseSpeed * 0.5f;
        }
    }
}

void ASLEntityResonant::Patrol(float DeltaTime)
{
    MovementSpeed = PatrolSpeed;

    if (PatrolPath.Num() == 0) return;

    FVector Target = PatrolPath[CurrentPatrolIndex];
    FVector Direction = (Target - GetActorLocation()).GetSafeNormal();
    AddMovementInput(Direction, 1.0f);

    if (FVector::Dist(GetActorLocation(), Target) < 100.0f)
    {
        CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPath.Num();
    }
}

void ASLEntityResonant::ChaseSound(float DeltaTime)
{
    FVector Direction = (LastHeardLocation - GetActorLocation()).GetSafeNormal();
    AddMovementInput(Direction, 1.0f);

    // If we reached the last heard location and no new sounds, investigate area
    if (FVector::Dist(GetActorLocation(), LastHeardLocation) < 150.0f)
    {
        if (ResonantState == EResonantState::Chasing)
        {
            ResonantState = EResonantState::Investigating;
        }
    }
}

ASLPlayerCharacter* ASLEntityResonant::FindTarget()
{
    // Override: Resonant only targets by sound, not by sight
    return nullptr; // Sound detection handled in ScanForSounds
}

#include "SLEntityBase.h"
#include "Player/SLPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

ASLEntityBase::ASLEntityBase()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
}

void ASLEntityBase::BeginPlay()
{
    Super::BeginPlay();
}

void ASLEntityBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsStunned)
    {
        StunTimer -= DeltaTime;
        if (StunTimer <= 0.0f)
        {
            OnStunEnd();
        }
        return;
    }

    UpdateBehavior(DeltaTime);
}

void ASLEntityBase::OnPlayerDetected(ASLPlayerCharacter* Player)
{
    CurrentTarget = Player;
    Player->AddPerceptionDrift(PerceptionDriftInflicted);
}

void ASLEntityBase::AttackPlayer(ASLPlayerCharacter* Player)
{
    if (!Player || Player->CurrentState != EPlayerState::Alive) return;
    Player->TakeDamageCustom(AttackDamage, EntityType);
}

void ASLEntityBase::Stun(float Duration)
{
    if (!bCanBeStunned) return;
    bIsStunned = true;
    StunTimer = Duration;
}

void ASLEntityBase::OnStunEnd()
{
    bIsStunned = false;
    StunTimer = 0.0f;
}

void ASLEntityBase::OnCorruptionPhaseChanged(ECorruptionPhase NewPhase)
{
    // Entities become more aggressive at higher phases
    if (NewPhase >= ECorruptionPhase::Critical)
    {
        MovementSpeed *= 1.3f;
        DetectionRange *= 1.5f;
    }
}

ASLPlayerCharacter* ASLEntityBase::FindTarget()
{
    ASLPlayerCharacter* BestTarget = nullptr;
    float BestScore = 0.0f;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ASLPlayerCharacter* Player = Cast<ASLPlayerCharacter>((*It)->GetPawn());
        if (!Player || Player->CurrentState != EPlayerState::Alive) continue;

        float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
        if (Distance > DetectionRange) continue;

        // Score based on distance, isolation, and noise
        float Score = (DetectionRange - Distance) / DetectionRange;
        if (Player->IsIsolated()) Score *= 2.0f;
        Score += Player->GetNoiseLevel() * 0.5f;

        if (Score > BestScore)
        {
            BestScore = Score;
            BestTarget = Player;
        }
    }

    return BestTarget;
}

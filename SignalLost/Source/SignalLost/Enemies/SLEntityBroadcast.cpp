#include "SLEntityBroadcast.h"
#include "Player/SLPlayerCharacter.h"
#include "Core/SLGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

ASLEntityBroadcast::ASLEntityBroadcast()
{
    EntityType = EEntityType::Broadcast;
    AttackDamage = 9999.0f; // Instantly lethal
    MovementSpeed = 200.0f;
    bCanBeStunned = false; // Unstoppable
    PerceptionDriftInflicted = 50.0f;
}

void ASLEntityBroadcast::UpdateBehavior(float DeltaTime)
{
    AdvanceTowardAirlock(DeltaTime);
    KillPlayersInPath();
    JamNearbyEquipment();
    ApplyVisualDistortion();
}

void ASLEntityBroadcast::AdvanceTowardAirlock(float DeltaTime)
{
    if (AirlockTarget.IsZero())
    {
        ASLGameMode* GM = Cast<ASLGameMode>(UGameplayStatics::GetGameMode(this));
        if (GM)
        {
            AirlockTarget = GM->AirlockLocation;
        }
        return;
    }

    FVector Direction = (AirlockTarget - GetActorLocation()).GetSafeNormal();
    FVector NewLocation = GetActorLocation() + Direction * AdvanceSpeed * DeltaTime;
    SetActorLocation(NewLocation);

    UE_LOG(LogSignalLost, Verbose, TEXT("Broadcast advancing. Distance to airlock: %.0f"),
        FVector::Dist(GetActorLocation(), AirlockTarget));
}

void ASLEntityBroadcast::KillPlayersInPath()
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ASLPlayerCharacter* Player = Cast<ASLPlayerCharacter>((*It)->GetPawn());
        if (!Player || Player->CurrentState != EPlayerState::Alive) continue;

        float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
        if (Distance < KillRadius)
        {
            UE_LOG(LogSignalLost, Error, TEXT("The Broadcast consumed a player!"));
            Player->TakeDamageCustom(AttackDamage, EntityType);
        }
    }
}

void ASLEntityBroadcast::JamNearbyEquipment()
{
    // All player equipment within JamRadius is disabled
    // This is checked by equipment components via IsLocationJammed()
}

bool ASLEntityBroadcast::IsLocationJammed(FVector Location) const
{
    return FVector::Dist(GetActorLocation(), Location) < JamRadius;
}

float ASLEntityBroadcast::GetDistortionIntensity(ASLPlayerCharacter* Player) const
{
    if (!Player) return 0.0f;

    float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
    if (Distance > DistortionRadius) return 0.0f;

    // Check if player is looking at The Broadcast
    FVector ToBroadcast = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal();
    FVector PlayerForward = Player->GetActorForwardVector();
    float LookDot = FVector::DotProduct(PlayerForward, ToBroadcast);

    if (LookDot < 0.3f) return 0.0f; // Not looking at it

    // Intensity scales with proximity and look angle
    float DistanceFactor = 1.0f - (Distance / DistortionRadius);
    float LookFactor = FMath::Clamp((LookDot - 0.3f) / 0.7f, 0.0f, 1.0f);

    return DistanceFactor * LookFactor;
}

void ASLEntityBroadcast::ApplyVisualDistortion()
{
    // Apply screen distortion post-process effects to nearby players
    // looking at The Broadcast. Handled via material parameter updates.
}

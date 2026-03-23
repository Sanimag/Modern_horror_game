#include "SLEntityEcho.h"
#include "Player/SLPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

ASLEntityEcho::ASLEntityEcho()
{
    EntityType = EEntityType::Echo;
    AttackDamage = 100.0f; // Instant down
    MovementSpeed = 800.0f; // Fast when lunging
    DetectionRange = 3000.0f;
    PerceptionDriftInflicted = 25.0f;
}

void ASLEntityEcho::UpdateBehavior(float DeltaTime)
{
    switch (EchoState)
    {
    case EEchoState::Idle:
    {
        CurrentTarget = FindTarget();
        if (CurrentTarget)
        {
            EchoState = EEchoState::Stalking;
            StalkTimer = 0.0f;
        }
        break;
    }

    case EEchoState::Stalking:
    {
        if (!CurrentTarget || CurrentTarget->CurrentState != EPlayerState::Alive)
        {
            EchoState = EEchoState::Idle;
            CurrentTarget = nullptr;
            break;
        }

        StalkTimer += DeltaTime;

        // Check if any player is looking at us
        bIsBeingObserved = false;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            ASLPlayerCharacter* Player = Cast<ASLPlayerCharacter>((*It)->GetPawn());
            if (Player && IsInPlayerView(Player))
            {
                bIsBeingObserved = true;
                break;
            }
        }

        // Only move when not observed
        if (!bIsBeingObserved)
        {
            MoveCloser(DeltaTime);
        }

        // Check for attack conditions
        if (StalkTimer >= MinStalkDuration && CurrentTarget->IsIsolated())
        {
            float DistToTarget = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
            if (DistToTarget < AttackDistance * 3.0f && !bIsBeingObserved)
            {
                EchoState = EEchoState::Lunging;
            }
        }
        break;
    }

    case EEchoState::Lunging:
    {
        if (!CurrentTarget) { EchoState = EEchoState::Idle; break; }

        // Rapid movement toward target
        FVector Direction = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        AddMovementInput(Direction, 1.0f);

        float DistToTarget = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
        if (DistToTarget < AttackDistance)
        {
            AttemptLunge();
        }
        break;
    }

    default:
        break;
    }
}

bool ASLEntityEcho::IsInPlayerView(ASLPlayerCharacter* Player) const
{
    if (!Player) return false;

    FVector ToEcho = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal();
    FVector PlayerForward = Player->GetActorForwardVector();

    float DotProduct = FVector::DotProduct(PlayerForward, ToEcho);

    // Within ~60 degree cone of vision and within flashlight range
    return DotProduct > 0.5f &&
           FVector::Dist(GetActorLocation(), Player->GetActorLocation()) < 2000.0f;
}

void ASLEntityEcho::MoveCloser(float DeltaTime)
{
    if (!CurrentTarget) return;

    float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());

    // Move to maintain ideal stalk distance, getting closer over time
    float DesiredDistance = FMath::Lerp(IdealStalkDistance, AttackDistance * 2.0f,
        FMath::Clamp(StalkTimer / MinStalkDuration, 0.0f, 1.0f));

    if (Distance > DesiredDistance)
    {
        FVector Direction = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        // Move in short bursts - teleport-like behavior
        float MoveAmount = FMath::Min(MovementSpeed * DeltaTime, Distance - DesiredDistance);
        SetActorLocation(GetActorLocation() + Direction * MoveAmount);
    }
}

void ASLEntityEcho::AttemptLunge()
{
    if (!CurrentTarget) return;

    UE_LOG(LogSignalLost, Warning, TEXT("Echo LUNGE on player!"));
    AttackPlayer(CurrentTarget);

    // After attack, reset and find new target
    EchoState = EEchoState::Idle;
    CurrentTarget = nullptr;
    StalkTimer = 0.0f;
}

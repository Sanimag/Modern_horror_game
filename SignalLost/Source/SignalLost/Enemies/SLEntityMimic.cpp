#include "SLEntityMimic.h"
#include "Player/SLPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

ASLEntityMimic::ASLEntityMimic()
{
    EntityType = EEntityType::Mimic;
    AttackDamage = 0.0f; // Uses latch DPS
    MovementSpeed = 0.0f; // Stationary when disguised
    bCanBeStunned = false;
    PerceptionDriftInflicted = 30.0f;
}

void ASLEntityMimic::UpdateBehavior(float DeltaTime)
{
    if (bIsDisguised) return; // Do nothing while disguised

    if (bIsLatched && LatchedPlayer)
    {
        // Deal DPS while latched
        LatchedPlayer->TakeDamageCustom(LatchDPS * DeltaTime, EntityType);

        // If player is alone, start dragging them down
        if (LatchedPlayer->IsIsolated() && !bDragging)
        {
            bDragging = true;
            DragTimer = DragDuration;
            UE_LOG(LogSignalLost, Warning, TEXT("Mimic dragging isolated player underground!"));
        }

        if (bDragging)
        {
            DragPlayerDown(DeltaTime);
        }
    }
}

void ASLEntityMimic::OnPlayerInteract(ASLPlayerCharacter* Player)
{
    if (!bIsDisguised || !Player) return;

    bIsDisguised = false;
    LatchOntoPlayer(Player);
    EmitAttractSignal();

    UE_LOG(LogSignalLost, Warning, TEXT("MIMIC TRIGGERED! Latched onto player!"));
}

void ASLEntityMimic::LatchOntoPlayer(ASLPlayerCharacter* Player)
{
    bIsLatched = true;
    LatchedPlayer = Player;

    // Force player to drop cores
    Player->DropAllCores();

    // Inflict perception drift from the shock
    Player->AddPerceptionDrift(PerceptionDriftInflicted);

    // Attach to player mesh
    AttachToActor(Player, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void ASLEntityMimic::DetachFromPlayer(ASLPlayerCharacter* Rescuer)
{
    if (!bIsLatched) return;

    bIsLatched = false;
    bDragging = false;
    LatchedPlayer = nullptr;
    DragTimer = 0.0f;

    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // Mimic is destroyed after being pulled off
    Destroy();

    UE_LOG(LogSignalLost, Log, TEXT("Mimic detached by teammate rescue"));
}

void ASLEntityMimic::DragPlayerDown(float DeltaTime)
{
    DragTimer -= DeltaTime;

    if (LatchedPlayer)
    {
        // Slowly sink the player into the floor
        float SinkRate = 30.0f; // Units per second
        FVector NewLocation = LatchedPlayer->GetActorLocation();
        NewLocation.Z -= SinkRate * DeltaTime;
        LatchedPlayer->SetActorLocation(NewLocation);
    }

    if (DragTimer <= 0.0f && LatchedPlayer)
    {
        // Player dragged underground — instant death
        LatchedPlayer->EnterSignalFade();
        bIsLatched = false;
        LatchedPlayer = nullptr;
        Destroy();

        UE_LOG(LogSignalLost, Error, TEXT("Mimic dragged player underground — DEAD"));
    }
}

void ASLEntityMimic::EmitAttractSignal()
{
    // Alert all entities within radius to converge on this location
    UE_LOG(LogSignalLost, Warning, TEXT("Mimic emitting attract signal radius %.0f"), SignalAttractRadius);
    // Implementation: iterate active entities and redirect them here
}

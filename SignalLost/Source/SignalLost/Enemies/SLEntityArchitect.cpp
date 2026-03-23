#include "SLEntityArchitect.h"
#include "Player/SLPlayerCharacter.h"
#include "World/SLStationGenerator.h"
#include "Core/SLGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

ASLEntityArchitect::ASLEntityArchitect()
{
    EntityType = EEntityType::Architect;
    AttackDamage = 0.0f; // Doesn't directly attack
    MovementSpeed = 0.0f; // Doesn't physically exist
    bCanBeStunned = false; // Can't be stunned — it's everywhere
    PerceptionDriftInflicted = 20.0f;
}

void ASLEntityArchitect::UpdateBehavior(float DeltaTime)
{
    ModificationTimer += DeltaTime;

    if (ModificationTimer < ModificationCooldown) return;

    ASLPlayerCharacter* Target = FindAirlockBoundPlayer();
    if (!Target) return;

    ModificationTimer = 0.0f;
    ModifyRoom();

    // Increase perception drift for players in modified areas
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ASLPlayerCharacter* Player = Cast<ASLPlayerCharacter>((*It)->GetPawn());
        if (!Player) continue;

        // Check if player is near a modified room
        // This would check against actual room bounds in full implementation
        Player->AddPerceptionDrift(5.0f);
    }
}

void ASLEntityArchitect::ModifyRoom()
{
    if (!StationGenerator) return;
    if (ModifiedRoomIDs.Num() >= MaxModifiedRooms) return;

    ASLPlayerCharacter* Target = FindAirlockBoundPlayer();
    if (!Target) return;

    // Choose modification type
    int32 ModType = FMath::RandRange(0, 2);
    switch (ModType)
    {
    case 0:
        CreateLoop(Target);
        break;
    case 1:
        ExtendAirlockPath(Target);
        break;
    case 2:
        // Swap door connections in a room near the target
        // Implementation depends on station generator's room system
        break;
    }

    UE_LOG(LogSignalLost, Warning, TEXT("Architect modified station layout. Modified rooms: %d"),
        ModifiedRoomIDs.Num());
}

void ASLEntityArchitect::CreateLoop(ASLPlayerCharacter* Target)
{
    // Make a corridor loop back to itself
    // The player walks forward but ends up where they started
    // Implemented via station generator room graph manipulation
    UE_LOG(LogSignalLost, Log, TEXT("Architect: Creating spatial loop near player"));
}

void ASLEntityArchitect::RotateRoom(FName RoomID)
{
    // Rotate a room 90/180 degrees while player isn't looking
    // Door connections shift, disorienting the player
    UE_LOG(LogSignalLost, Log, TEXT("Architect: Rotating room %s"), *RoomID.ToString());
    ModifiedRoomIDs.AddUnique(RoomID);
}

void ASLEntityArchitect::SwapDoorConnections(FName RoomID)
{
    // Door A now leads where Door B used to, and vice versa
    UE_LOG(LogSignalLost, Log, TEXT("Architect: Swapping doors in room %s"), *RoomID.ToString());
    ModifiedRoomIDs.AddUnique(RoomID);
}

void ASLEntityArchitect::ExtendAirlockPath(ASLPlayerCharacter* Target)
{
    // Insert additional rooms between the target and the airlock
    // Increases the physical distance to extraction
    UE_LOG(LogSignalLost, Log, TEXT("Architect: Extending path to airlock"));
}

ASLPlayerCharacter* ASLEntityArchitect::FindAirlockBoundPlayer()
{
    ASLGameMode* GM = Cast<ASLGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GM) return nullptr;

    ASLPlayerCharacter* BestTarget = nullptr;
    float BestScore = 0.0f;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ASLPlayerCharacter* Player = Cast<ASLPlayerCharacter>((*It)->GetPawn());
        if (!Player || Player->CurrentState != EPlayerState::Alive) continue;

        // Score: players heading toward airlock are priority targets
        FVector ToAirlock = (GM->AirlockLocation - Player->GetActorLocation()).GetSafeNormal();
        FVector PlayerVelocity = Player->GetVelocity().GetSafeNormal();

        float HeadingScore = FVector::DotProduct(ToAirlock, PlayerVelocity);
        if (HeadingScore > BestScore)
        {
            BestScore = HeadingScore;
            BestTarget = Player;
        }
    }

    return BestTarget;
}

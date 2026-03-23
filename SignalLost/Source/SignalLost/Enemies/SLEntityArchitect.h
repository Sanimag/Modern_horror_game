#pragma once

#include "SLEntityBase.h"
#include "SLEntityArchitect.generated.h"

class ASLStationGenerator;

/**
 * THE ARCHITECT (Environment Manipulator)
 * Never seen directly. Rearranges the station around players.
 * Hallways loop, rooms rotate, doors open onto walls.
 * Activates above 50% Corruption. Targets players navigating to airlock.
 * Counter: Scanner detects modified rooms. Physical markers track changes.
 */
UCLASS()
class SIGNALLOST_API ASLEntityArchitect : public ASLEntityBase
{
    GENERATED_BODY()

public:
    ASLEntityArchitect();

    /** How often the Architect can modify a room (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Architect")
    float ModificationCooldown = 20.0f;

    /** Radius around target player where modifications occur */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Architect")
    float ModificationRadius = 3000.0f;

    /** Max rooms that can be modified simultaneously */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Architect")
    int32 MaxModifiedRooms = 5;

    /** Reference to station generator for room manipulation */
    UPROPERTY(BlueprintReadWrite, Category = "Architect")
    ASLStationGenerator* StationGenerator = nullptr;

    /** IDs of currently modified rooms */
    UPROPERTY(BlueprintReadOnly, Category = "Architect")
    TArray<FName> ModifiedRoomIDs;

protected:
    virtual void UpdateBehavior(float DeltaTime) override;

private:
    void ModifyRoom();
    void CreateLoop(ASLPlayerCharacter* Target);
    void RotateRoom(FName RoomID);
    void SwapDoorConnections(FName RoomID);
    void ExtendAirlockPath(ASLPlayerCharacter* Target);

    float ModificationTimer = 0.0f;

    // The Architect targets players heading toward the airlock
    ASLPlayerCharacter* FindAirlockBoundPlayer();
};

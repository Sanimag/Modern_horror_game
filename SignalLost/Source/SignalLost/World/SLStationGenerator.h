#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/SLGameTypes.h"
#include "SLStationGenerator.generated.h"

/**
 * Procedural Station Generator
 * Assembles underwater relay stations from hand-crafted room modules
 * connected by procedurally generated corridors.
 * Room placement, loot distribution, and entity spawn points are randomized.
 * Each run feels familiar in vocabulary but unique in composition.
 */
UCLASS()
class SIGNALLOST_API ASLStationGenerator : public AActor
{
    GENERATED_BODY()

public:
    ASLStationGenerator();

    // ========================================================================
    // GENERATION
    // ========================================================================

    /** Generate a complete station layout */
    UFUNCTION(BlueprintCallable, Category = "Generation")
    void GenerateStation(int32 Seed, int32 Depth, EMissionType MissionType);

    /** Get the generated room layout */
    UFUNCTION(BlueprintPure, Category = "Generation")
    TArray<FRoomModule> GetRooms() const { return GeneratedRooms; }

    /** Get airlock world position */
    UFUNCTION(BlueprintPure, Category = "Generation")
    FVector GetAirlockPosition() const;

    /** Get spawn points for signal cores */
    UFUNCTION(BlueprintPure, Category = "Generation")
    TArray<FVector> GetCoreSpawnPoints() const;

    /** Get valid entity spawn points */
    UFUNCTION(BlueprintPure, Category = "Generation")
    TArray<FVector> GetEntitySpawnPoints() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /** Room module data assets for each room type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
    TMap<ERoomType, TArray<FName>> RoomModuleLibrary;

    /** Minimum rooms per station */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
    int32 MinRooms = 15;

    /** Maximum rooms per station */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
    int32 MaxRooms = 40;

    /** Corridor length range */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
    FVector2D CorridorLengthRange = FVector2D(500.0f, 2000.0f);

    /** Room spacing */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
    float MinRoomSpacing = 200.0f;

    /** Depth affects station size and complexity */
    UPROPERTY(BlueprintReadOnly, Category = "Generation")
    int32 StationDepth = 1;

    // ========================================================================
    // ARCHITECT MANIPULATION
    // ========================================================================

    /** Modify a room's connections (used by The Architect entity) */
    UFUNCTION(BlueprintCallable, Category = "Architect")
    void ModifyRoomConnections(FName RoomID);

    /** Insert a new room between two existing rooms */
    UFUNCTION(BlueprintCallable, Category = "Architect")
    void InsertRoom(FName BetweenRoomA, FName BetweenRoomB, const FRoomModule& NewRoom);

    /** Create a spatial loop (corridor leads back to itself) */
    UFUNCTION(BlueprintCallable, Category = "Architect")
    void CreateSpatialLoop(FName RoomID);

    /** Seal a doorway */
    UFUNCTION(BlueprintCallable, Category = "Architect")
    void SealDoorway(FName RoomID, int32 DoorwayIndex);

    /** Open a new doorway in a wall */
    UFUNCTION(BlueprintCallable, Category = "Architect")
    void OpenNewDoorway(FName RoomID, FVector WallPosition);

    /** Fail a random light in the station */
    UFUNCTION(BlueprintCallable, Category = "Lighting")
    void FailRandomLight();

    /** Is a room modified by The Architect? */
    UFUNCTION(BlueprintPure, Category = "Architect")
    bool IsRoomModified(FName RoomID) const;

    /** Get number of failed lights */
    UFUNCTION(BlueprintPure, Category = "Lighting")
    int32 GetFailedLightCount() const { return FailedLights.Num(); }

protected:
    virtual void BeginPlay() override;

private:
    // ========================================================================
    // GENERATION INTERNALS
    // ========================================================================

    /** Place the airlock as the entry point */
    void PlaceAirlock();

    /** Generate the main branch from airlock */
    void GenerateMainBranch(int32 RoomCount);

    /** Generate side branches for exploration */
    void GenerateSideBranches(int32 BranchCount);

    /** Connect rooms with corridors */
    void ConnectRooms(FName RoomA, FName RoomB);

    /** Place loot throughout the station */
    void DistributeLoot(EMissionType MissionType);

    /** Place entity spawn points */
    void DistributeEntitySpawns();

    /** Validate the generated layout (no overlaps, all connected) */
    bool ValidateLayout() const;

    /** Select a room type appropriate for the current depth */
    ERoomType SelectRoomType(int32 DistanceFromAirlock) const;

    // ========================================================================
    // ROOM GRAPH
    // ========================================================================

    struct FRoomNode
    {
        FName RoomID;
        FRoomModule Module;
        FVector WorldPosition = FVector::ZeroVector;
        FRotator WorldRotation = FRotator::ZeroRotator;
        TArray<FName> ConnectedRoomIDs;
        bool bIsModifiedByArchitect = false;
        int32 DepthFromAirlock = 0;
    };

    TMap<FName, FRoomNode> RoomGraph;
    TArray<FRoomModule> GeneratedRooms;
    FName AirlockRoomID;

    // Random stream for deterministic generation
    FRandomStream RandomStream;

    // Tracking
    TArray<FName> ModifiedRooms;
    TArray<int32> FailedLights;
    int32 TotalLightCount = 0;

    // Loot placement
    TArray<FVector> CoreSpawnPoints;
    TArray<FVector> EntitySpawnPoints;
};

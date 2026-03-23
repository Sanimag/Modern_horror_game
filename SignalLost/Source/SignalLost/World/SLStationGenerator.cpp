#include "SLStationGenerator.h"
#include "SignalLost.h"

ASLStationGenerator::ASLStationGenerator()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
}

void ASLStationGenerator::BeginPlay()
{
    Super::BeginPlay();
}

void ASLStationGenerator::GenerateStation(int32 Seed, int32 Depth, EMissionType MissionType)
{
    RandomStream.Initialize(Seed);
    StationDepth = Depth;

    // Clear previous generation
    RoomGraph.Empty();
    GeneratedRooms.Empty();
    CoreSpawnPoints.Empty();
    EntitySpawnPoints.Empty();
    ModifiedRooms.Empty();
    FailedLights.Empty();

    // Scale room count with depth
    int32 RoomCount = FMath::Clamp(
        MinRooms + (Depth * 5) + RandomStream.RandRange(-3, 3),
        MinRooms, MaxRooms);

    UE_LOG(LogSignalLost, Log, TEXT("Generating station: Seed=%d, Depth=%d, Rooms=%d, Mission=%d"),
        Seed, Depth, RoomCount, (int32)MissionType);

    // Phase 1: Place airlock
    PlaceAirlock();

    // Phase 2: Generate main branch (critical path)
    int32 MainBranchRooms = RoomCount * 0.6f;
    GenerateMainBranch(MainBranchRooms);

    // Phase 3: Generate side branches (exploration content)
    int32 SideBranchRooms = RoomCount - MainBranchRooms - 1; // -1 for airlock
    int32 BranchCount = FMath::Max(2, SideBranchRooms / 3);
    GenerateSideBranches(BranchCount);

    // Phase 4: Distribute loot and entity spawns
    DistributeLoot(MissionType);
    DistributeEntitySpawns();

    // Phase 5: Validate
    if (ValidateLayout())
    {
        UE_LOG(LogSignalLost, Log, TEXT("Station generated successfully: %d rooms, %d cores, %d entity spawns"),
            RoomGraph.Num(), CoreSpawnPoints.Num(), EntitySpawnPoints.Num());
    }
    else
    {
        UE_LOG(LogSignalLost, Error, TEXT("Station validation failed! Regenerating..."));
        GenerateStation(Seed + 1, Depth, MissionType);
    }
}

void ASLStationGenerator::PlaceAirlock()
{
    FRoomNode AirlockNode;
    AirlockNode.RoomID = FName(TEXT("Airlock_Main"));
    AirlockNode.Module.RoomID = AirlockNode.RoomID;
    AirlockNode.Module.RoomType = ERoomType::Airlock;
    AirlockNode.Module.RoomSize = FVector(800.0f, 600.0f, 400.0f);
    AirlockNode.Module.MaxDoorways = 2;
    AirlockNode.Module.bHasEmergencyLighting = true;
    AirlockNode.Module.bIsArchitectModifiable = false; // Airlock can't be modified
    AirlockNode.WorldPosition = FVector::ZeroVector;
    AirlockNode.DepthFromAirlock = 0;

    AirlockRoomID = AirlockNode.RoomID;
    RoomGraph.Add(AirlockNode.RoomID, AirlockNode);
    GeneratedRooms.Add(AirlockNode.Module);
}

void ASLStationGenerator::GenerateMainBranch(int32 RoomCount)
{
    FName PreviousRoomID = AirlockRoomID;
    FVector CurrentPosition = FVector::ZeroVector;

    for (int32 i = 0; i < RoomCount; i++)
    {
        // Generate corridor offset
        float CorridorLength = RandomStream.FRandRange(
            CorridorLengthRange.X, CorridorLengthRange.Y);

        // Random direction (primarily forward with some lateral variation)
        FVector Direction;
        float Angle = RandomStream.FRandRange(-45.0f, 45.0f);
        Direction.X = FMath::Cos(FMath::DegreesToRadians(Angle));
        Direction.Y = FMath::Sin(FMath::DegreesToRadians(Angle));
        Direction.Z = RandomStream.FRandRange(-0.1f, -0.05f); // Slight downward trend (deeper)

        CurrentPosition += Direction.GetSafeNormal() * CorridorLength;

        // Select room type based on depth
        int32 DepthFromAirlock = i + 1;
        ERoomType RoomType = SelectRoomType(DepthFromAirlock);

        // Create room node
        FRoomNode NewNode;
        NewNode.RoomID = FName(*FString::Printf(TEXT("Main_%d_%s"), i,
            *UEnum::GetValueAsString(RoomType)));
        NewNode.Module.RoomID = NewNode.RoomID;
        NewNode.Module.RoomType = RoomType;
        NewNode.WorldPosition = CurrentPosition;
        NewNode.DepthFromAirlock = DepthFromAirlock;

        // Set room properties based on type
        switch (RoomType)
        {
        case ERoomType::ProcessingHall:
            NewNode.Module.RoomSize = FVector(1500.0f, 1200.0f, 500.0f);
            NewNode.Module.MaxDoorways = 4;
            NewNode.Module.bHasEmergencyLighting = true;
            break;
        case ERoomType::ServerCrypt:
            NewNode.Module.RoomSize = FVector(1000.0f, 800.0f, 300.0f);
            NewNode.Module.MaxDoorways = 2;
            break;
        case ERoomType::MaintenanceTunnel:
            NewNode.Module.RoomSize = FVector(2000.0f, 300.0f, 250.0f);
            NewNode.Module.MaxDoorways = 2;
            break;
        case ERoomType::PressureChamber:
            NewNode.Module.RoomSize = FVector(600.0f, 600.0f, 600.0f);
            NewNode.Module.MaxDoorways = 3;
            NewNode.Module.bHasEmergencyLighting = true;
            break;
        case ERoomType::ObservationDome:
            NewNode.Module.RoomSize = FVector(1200.0f, 1200.0f, 800.0f);
            NewNode.Module.MaxDoorways = 2;
            break;
        case ERoomType::ControlRoom:
            NewNode.Module.RoomSize = FVector(800.0f, 800.0f, 400.0f);
            NewNode.Module.MaxDoorways = 3;
            NewNode.Module.bHasEmergencyLighting = true;
            break;
        default:
            NewNode.Module.RoomSize = FVector(1000.0f, 1000.0f, 400.0f);
            NewNode.Module.MaxDoorways = 4;
            break;
        }

        // Generate spawn points within the room
        int32 LootPoints = RandomStream.RandRange(1, 3);
        for (int32 j = 0; j < LootPoints; j++)
        {
            FVector Offset(
                RandomStream.FRandRange(-NewNode.Module.RoomSize.X * 0.3f, NewNode.Module.RoomSize.X * 0.3f),
                RandomStream.FRandRange(-NewNode.Module.RoomSize.Y * 0.3f, NewNode.Module.RoomSize.Y * 0.3f),
                0.0f
            );
            NewNode.Module.LootSpawnPoints.Add(CurrentPosition + Offset);
        }

        int32 EntityPoints = RandomStream.RandRange(0, 2);
        for (int32 j = 0; j < EntityPoints; j++)
        {
            FVector Offset(
                RandomStream.FRandRange(-NewNode.Module.RoomSize.X * 0.4f, NewNode.Module.RoomSize.X * 0.4f),
                RandomStream.FRandRange(-NewNode.Module.RoomSize.Y * 0.4f, NewNode.Module.RoomSize.Y * 0.4f),
                0.0f
            );
            NewNode.Module.EntitySpawnPoints.Add(CurrentPosition + Offset);
        }

        // Connect to previous room
        NewNode.ConnectedRoomIDs.Add(PreviousRoomID);
        RoomGraph.FindChecked(PreviousRoomID).ConnectedRoomIDs.Add(NewNode.RoomID);

        RoomGraph.Add(NewNode.RoomID, NewNode);
        GeneratedRooms.Add(NewNode.Module);

        PreviousRoomID = NewNode.RoomID;
        TotalLightCount += (NewNode.Module.bHasEmergencyLighting ? 4 : 1);
    }
}

void ASLStationGenerator::GenerateSideBranches(int32 BranchCount)
{
    // Collect main branch rooms as potential branch points
    TArray<FName> MainBranchRooms;
    for (auto& Pair : RoomGraph)
    {
        if (Pair.Key.ToString().Contains(TEXT("Main_")))
        {
            MainBranchRooms.Add(Pair.Key);
        }
    }

    for (int32 b = 0; b < BranchCount && MainBranchRooms.Num() > 0; b++)
    {
        // Pick a random main branch room to branch from
        int32 BranchPointIdx = RandomStream.RandRange(0, MainBranchRooms.Num() - 1);
        FName BranchPointID = MainBranchRooms[BranchPointIdx];
        FRoomNode& BranchPoint = RoomGraph.FindChecked(BranchPointID);

        if (BranchPoint.ConnectedRoomIDs.Num() >= BranchPoint.Module.MaxDoorways) continue;

        int32 BranchLength = RandomStream.RandRange(2, 5);
        FName PrevID = BranchPointID;
        FVector Pos = BranchPoint.WorldPosition;

        for (int32 r = 0; r < BranchLength; r++)
        {
            float CorridorLen = RandomStream.FRandRange(CorridorLengthRange.X, CorridorLengthRange.Y);
            float Angle = RandomStream.FRandRange(60.0f, 120.0f) * (RandomStream.FRand() > 0.5f ? 1.0f : -1.0f);

            FVector Dir;
            Dir.X = FMath::Cos(FMath::DegreesToRadians(Angle));
            Dir.Y = FMath::Sin(FMath::DegreesToRadians(Angle));
            Dir.Z = 0.0f;
            Pos += Dir.GetSafeNormal() * CorridorLen;

            int32 Depth = BranchPoint.DepthFromAirlock + r + 1;
            ERoomType RType = SelectRoomType(Depth);

            FRoomNode Node;
            Node.RoomID = FName(*FString::Printf(TEXT("Side_%d_%d"), b, r));
            Node.Module.RoomID = Node.RoomID;
            Node.Module.RoomType = RType;
            Node.Module.RoomSize = FVector(800.0f, 800.0f, 350.0f);
            Node.Module.MaxDoorways = 3;
            Node.WorldPosition = Pos;
            Node.DepthFromAirlock = Depth;

            // Loot in side branches (higher value deeper)
            int32 LootCount = RandomStream.RandRange(1, 4);
            for (int32 j = 0; j < LootCount; j++)
            {
                FVector Offset(RandomStream.FRandRange(-300.0f, 300.0f),
                    RandomStream.FRandRange(-300.0f, 300.0f), 0.0f);
                Node.Module.LootSpawnPoints.Add(Pos + Offset);
            }

            Node.ConnectedRoomIDs.Add(PrevID);
            RoomGraph.FindChecked(PrevID).ConnectedRoomIDs.Add(Node.RoomID);

            RoomGraph.Add(Node.RoomID, Node);
            GeneratedRooms.Add(Node.Module);
            PrevID = Node.RoomID;
        }
    }
}

void ASLStationGenerator::ConnectRooms(FName RoomA, FName RoomB)
{
    if (RoomGraph.Contains(RoomA) && RoomGraph.Contains(RoomB))
    {
        RoomGraph.FindChecked(RoomA).ConnectedRoomIDs.AddUnique(RoomB);
        RoomGraph.FindChecked(RoomB).ConnectedRoomIDs.AddUnique(RoomA);
    }
}

void ASLStationGenerator::DistributeLoot(EMissionType MissionType)
{
    CoreSpawnPoints.Empty();

    for (auto& Pair : RoomGraph)
    {
        for (const FVector& SpawnPoint : Pair.Value.Module.LootSpawnPoints)
        {
            // Higher value cores spawn deeper
            if (Pair.Value.DepthFromAirlock > 3)
            {
                CoreSpawnPoints.Add(SpawnPoint);
            }
            else if (RandomStream.FRand() < 0.5f)
            {
                CoreSpawnPoints.Add(SpawnPoint);
            }
        }
    }

    // Mission-specific loot placement
    switch (MissionType)
    {
    case EMissionType::DataRecovery:
        // Place 3 terminals in high-corruption zones (deep rooms)
        break;
    case EMissionType::BlackBox:
        // Place flight recorder at the deepest point
        break;
    default:
        break;
    }
}

void ASLStationGenerator::DistributeEntitySpawns()
{
    EntitySpawnPoints.Empty();
    for (auto& Pair : RoomGraph)
    {
        for (const FVector& SpawnPoint : Pair.Value.Module.EntitySpawnPoints)
        {
            EntitySpawnPoints.Add(SpawnPoint);
        }
    }
}

bool ASLStationGenerator::ValidateLayout() const
{
    // Check all rooms are reachable from airlock via BFS
    if (!RoomGraph.Contains(AirlockRoomID)) return false;

    TSet<FName> Visited;
    TQueue<FName> Queue;
    Queue.Enqueue(AirlockRoomID);
    Visited.Add(AirlockRoomID);

    while (!Queue.IsEmpty())
    {
        FName Current;
        Queue.Dequeue(Current);

        const FRoomNode& Node = RoomGraph.FindChecked(Current);
        for (const FName& Connected : Node.ConnectedRoomIDs)
        {
            if (!Visited.Contains(Connected))
            {
                Visited.Add(Connected);
                Queue.Enqueue(Connected);
            }
        }
    }

    return Visited.Num() == RoomGraph.Num();
}

ERoomType ASLStationGenerator::SelectRoomType(int32 DistanceFromAirlock) const
{
    // Near airlock: Control rooms, junctions
    // Mid: Processing halls, pressure chambers
    // Deep: Server crypts, observation domes, maintenance tunnels

    TArray<ERoomType> ValidTypes;

    if (DistanceFromAirlock <= 3)
    {
        ValidTypes = { ERoomType::ControlRoom, ERoomType::Junction, ERoomType::Corridor };
    }
    else if (DistanceFromAirlock <= 7)
    {
        ValidTypes = { ERoomType::ProcessingHall, ERoomType::PressureChamber,
                       ERoomType::Junction, ERoomType::MaintenanceTunnel };
    }
    else
    {
        ValidTypes = { ERoomType::ServerCrypt, ERoomType::ObservationDome,
                       ERoomType::MaintenanceTunnel, ERoomType::ProcessingHall };
    }

    return ValidTypes[const_cast<FRandomStream&>(RandomStream).RandRange(0, ValidTypes.Num() - 1)];
}

FVector ASLStationGenerator::GetAirlockPosition() const
{
    if (RoomGraph.Contains(AirlockRoomID))
    {
        return RoomGraph.FindChecked(AirlockRoomID).WorldPosition;
    }
    return FVector::ZeroVector;
}

TArray<FVector> ASLStationGenerator::GetCoreSpawnPoints() const
{
    return CoreSpawnPoints;
}

TArray<FVector> ASLStationGenerator::GetEntitySpawnPoints() const
{
    return EntitySpawnPoints;
}

// ============================================================================
// ARCHITECT MANIPULATION
// ============================================================================

void ASLStationGenerator::ModifyRoomConnections(FName RoomID)
{
    if (!RoomGraph.Contains(RoomID)) return;
    FRoomNode& Room = RoomGraph.FindChecked(RoomID);
    if (!Room.Module.bIsArchitectModifiable) return;

    Room.bIsModifiedByArchitect = true;
    ModifiedRooms.AddUnique(RoomID);
}

void ASLStationGenerator::InsertRoom(FName BetweenRoomA, FName BetweenRoomB, const FRoomModule& NewRoom)
{
    if (!RoomGraph.Contains(BetweenRoomA) || !RoomGraph.Contains(BetweenRoomB)) return;

    // Create new room node between the two
    FRoomNode NewNode;
    NewNode.RoomID = NewRoom.RoomID;
    NewNode.Module = NewRoom;
    NewNode.WorldPosition = (RoomGraph.FindChecked(BetweenRoomA).WorldPosition +
                             RoomGraph.FindChecked(BetweenRoomB).WorldPosition) * 0.5f;
    NewNode.bIsModifiedByArchitect = true;

    // Disconnect A-B, connect A-New-B
    RoomGraph.FindChecked(BetweenRoomA).ConnectedRoomIDs.Remove(BetweenRoomB);
    RoomGraph.FindChecked(BetweenRoomB).ConnectedRoomIDs.Remove(BetweenRoomA);

    NewNode.ConnectedRoomIDs.Add(BetweenRoomA);
    NewNode.ConnectedRoomIDs.Add(BetweenRoomB);
    RoomGraph.FindChecked(BetweenRoomA).ConnectedRoomIDs.Add(NewNode.RoomID);
    RoomGraph.FindChecked(BetweenRoomB).ConnectedRoomIDs.Add(NewNode.RoomID);

    RoomGraph.Add(NewNode.RoomID, NewNode);
    GeneratedRooms.Add(NewRoom);
    ModifiedRooms.AddUnique(NewNode.RoomID);
}

void ASLStationGenerator::CreateSpatialLoop(FName RoomID)
{
    if (!RoomGraph.Contains(RoomID)) return;

    // Make one of the room's exits loop back to itself
    FRoomNode& Room = RoomGraph.FindChecked(RoomID);
    Room.ConnectedRoomIDs.Add(RoomID); // Self-loop
    Room.bIsModifiedByArchitect = true;
    ModifiedRooms.AddUnique(RoomID);

    UE_LOG(LogSignalLost, Warning, TEXT("Architect: Spatial loop created in room %s"), *RoomID.ToString());
}

void ASLStationGenerator::SealDoorway(FName RoomID, int32 DoorwayIndex)
{
    if (!RoomGraph.Contains(RoomID)) return;

    FRoomNode& Room = RoomGraph.FindChecked(RoomID);
    if (Room.ConnectedRoomIDs.IsValidIndex(DoorwayIndex))
    {
        FName DisconnectedRoom = Room.ConnectedRoomIDs[DoorwayIndex];
        Room.ConnectedRoomIDs.RemoveAt(DoorwayIndex);

        // Also remove reverse connection
        if (RoomGraph.Contains(DisconnectedRoom))
        {
            RoomGraph.FindChecked(DisconnectedRoom).ConnectedRoomIDs.Remove(RoomID);
        }

        Room.bIsModifiedByArchitect = true;
        ModifiedRooms.AddUnique(RoomID);
    }
}

void ASLStationGenerator::OpenNewDoorway(FName RoomID, FVector WallPosition)
{
    if (!RoomGraph.Contains(RoomID)) return;

    FRoomNode& Room = RoomGraph.FindChecked(RoomID);
    Room.Module.DoorwayPositions.Add(WallPosition);
    Room.bIsModifiedByArchitect = true;
    ModifiedRooms.AddUnique(RoomID);
}

void ASLStationGenerator::FailRandomLight()
{
    if (TotalLightCount <= 0) return;

    int32 LightIndex = FMath::RandRange(0, TotalLightCount - 1);
    if (!FailedLights.Contains(LightIndex))
    {
        FailedLights.Add(LightIndex);
        UE_LOG(LogSignalLost, Log, TEXT("Light %d failed. Total failed: %d/%d"),
            LightIndex, FailedLights.Num(), TotalLightCount);
    }
}

bool ASLStationGenerator::IsRoomModified(FName RoomID) const
{
    return ModifiedRooms.Contains(RoomID);
}

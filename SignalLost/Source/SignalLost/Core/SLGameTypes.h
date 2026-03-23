#pragma once

#include "CoreMinimal.h"
#include "SLGameTypes.generated.h"

// ============================================================================
// ENUMS
// ============================================================================

UENUM(BlueprintType)
enum class ECorruptionPhase : uint8
{
    Quiet      UMETA(DisplayName = "Quiet (0-24%)"),
    Stirring   UMETA(DisplayName = "Stirring (25-49%)"),
    Active     UMETA(DisplayName = "Active (50-74%)"),
    Critical   UMETA(DisplayName = "Critical (75-99%)"),
    Cascade    UMETA(DisplayName = "Cascade (100%)")
};

UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
    Scanner    UMETA(DisplayName = "Scanner"),
    Technician UMETA(DisplayName = "Technician"),
    Warden     UMETA(DisplayName = "Warden"),
    Courier    UMETA(DisplayName = "Courier"),
    Operator   UMETA(DisplayName = "Operator"),
    None       UMETA(DisplayName = "Unassigned")
};

UENUM(BlueprintType)
enum class EEntityType : uint8
{
    Echo       UMETA(DisplayName = "The Echo"),
    Resonant   UMETA(DisplayName = "The Resonant"),
    Architect  UMETA(DisplayName = "The Architect"),
    Mimic      UMETA(DisplayName = "The Mimic"),
    Broadcast  UMETA(DisplayName = "The Broadcast")
};

UENUM(BlueprintType)
enum class EEquipmentType : uint8
{
    Flashlight,
    UVLantern,
    MotionScanner,
    ResyncKit,
    NoiseMaker,
    SignalJammer,
    GlowStick,
    ReinforcedHarness,
    EmergencyBeacon,
    PulseRadar,
    ExtractionAccelerator,
    SignalFlareGun,
    MagneticHarness,
    RemoteDrone
};

UENUM(BlueprintType)
enum class EMissionType : uint8
{
    StandardSalvage  UMETA(DisplayName = "Standard Salvage"),
    DataRecovery     UMETA(DisplayName = "Data Recovery"),
    RescueOp         UMETA(DisplayName = "Rescue Op"),
    PurgeContract    UMETA(DisplayName = "Purge Contract"),
    BlackBox         UMETA(DisplayName = "Black Box")
};

UENUM(BlueprintType)
enum class EPlayerState : uint8
{
    Alive,
    SignalFade,  // Downed, 60s to revive
    Dead,
    Broadcasting // Sacrificed to reveal entities
};

UENUM(BlueprintType)
enum class ERoomType : uint8
{
    ProcessingHall,
    ServerCrypt,
    MaintenanceTunnel,
    PressureChamber,
    ObservationDome,
    ControlRoom,
    Junction,
    Airlock,
    Corridor
};

// ============================================================================
// STRUCTS
// ============================================================================

USTRUCT(BlueprintType)
struct FSignalCore
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName CoreID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CreditValue = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CorruptionCost = 10.0f; // % added to Corruption Index on extraction

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Weight = 1.0f; // Movement speed penalty multiplier

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Integrity = 100.0f; // Damaged cores worth less

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequiresMultiplePlayers = false;
};

USTRUCT(BlueprintType)
struct FMissionContract
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ContractID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMissionType MissionType = EMissionType::StandardSalvage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ThreatRating = 1; // 1-5

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PayoutMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseCorruption = 0.0f; // Starting corruption %

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinDepth = 1; // Station depth tier

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EEntityType> GuaranteedEntities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;
};

USTRUCT(BlueprintType)
struct FRoomModule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RoomID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERoomType RoomType = ERoomType::Corridor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector RoomSize = FVector(1000.0f, 1000.0f, 400.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxDoorways = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> DoorwayPositions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> LootSpawnPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> EntitySpawnPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasEmergencyLighting = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsArchitectModifiable = true;
};

USTRUCT(BlueprintType)
struct FEquipmentData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EEquipmentType Type = EEquipmentType::Flashlight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PurchaseCost = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxCharges = -1; // -1 = unlimited / battery-based

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Durability = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BatteryLife = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WeightPenalty = 0.0f; // Movement speed reduction

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 UpgradeLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxUpgradeLevel = 3;
};

USTRUCT(BlueprintType)
struct FSubmarineUpgrade
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName UpgradeID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Cost = 500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxLevel = 5;
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCorruptionChanged, float, NewCorruption, ECorruptionPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPerceptionDriftChanged, APlayerController*, Player, float, NewDrift);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntitySpawned, EEntityType, EntityType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerStateChanged, APlayerController*, Player, EPlayerState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCascadeTriggered, float, CountdownSeconds);

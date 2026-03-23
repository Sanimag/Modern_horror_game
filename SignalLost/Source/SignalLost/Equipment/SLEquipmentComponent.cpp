#include "SLEquipmentComponent.h"
#include "Player/SLPlayerCharacter.h"
#include "Core/SLGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

USLEquipmentComponent::USLEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void USLEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();
}

void USLEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    TickBatteryDrain(DeltaTime);
    TickEquipmentFailures(DeltaTime);

    if (bIsResyncing)
    {
        TickResync(DeltaTime);
    }
}

// ============================================================================
// INVENTORY
// ============================================================================

bool USLEquipmentComponent::AddEquipment(const FEquipmentData& Equipment)
{
    if (Inventory.Num() >= MaxInventorySlots) return false;
    Inventory.Add(Equipment);
    return true;
}

bool USLEquipmentComponent::RemoveEquipment(int32 Index)
{
    if (!Inventory.IsValidIndex(Index)) return false;
    Inventory.RemoveAt(Index);
    if (ActiveEquipmentIndex >= Inventory.Num())
    {
        ActiveEquipmentIndex = Inventory.Num() - 1;
    }
    return true;
}

void USLEquipmentComponent::SwitchEquipment(int32 Index)
{
    if (Inventory.IsValidIndex(Index))
    {
        ActiveEquipmentIndex = Index;
    }
}

FEquipmentData USLEquipmentComponent::GetActiveEquipment() const
{
    if (Inventory.IsValidIndex(ActiveEquipmentIndex))
    {
        return Inventory[ActiveEquipmentIndex];
    }
    return FEquipmentData();
}

bool USLEquipmentComponent::HasEquipment(EEquipmentType Type) const
{
    for (const FEquipmentData& Item : Inventory)
    {
        if (Item.Type == Type) return true;
    }
    return false;
}

// ============================================================================
// USAGE
// ============================================================================

bool USLEquipmentComponent::UseActiveEquipment()
{
    if (!Inventory.IsValidIndex(ActiveEquipmentIndex)) return false;
    if (bIsJammed) return false;
    if (IsEquipmentMalfunctioning()) return false;

    FEquipmentData& Equipment = Inventory[ActiveEquipmentIndex];

    // Check battery
    if (Equipment.BatteryLife <= 0.0f && Equipment.MaxCharges == -1) return false;

    // Check charges
    if (Equipment.MaxCharges > 0 && Equipment.MaxCharges <= 0) return false;

    switch (Equipment.Type)
    {
    case EEquipmentType::Flashlight:
        ToggleFlashlight();
        break;
    case EEquipmentType::PulseRadar:
        PulseRadarScan();
        break;
    case EEquipmentType::SignalFlareGun:
        // Direction from player aim
        break;
    case EEquipmentType::NoiseMaker:
        // ThrowNoiseMaker with aim direction
        break;
    case EEquipmentType::SignalJammer:
        ActivateSignalJammer();
        break;
    case EEquipmentType::GlowStick:
        DropGlowStick();
        break;
    case EEquipmentType::EmergencyBeacon:
        ActivateEmergencyBeacon();
        break;
    case EEquipmentType::ResyncKit:
        // Need a target - handled by interaction system
        break;
    default:
        break;
    }

    return true;
}

bool USLEquipmentComponent::ConsumeCharge(int32 EquipmentIndex)
{
    if (!Inventory.IsValidIndex(EquipmentIndex)) return false;

    FEquipmentData& Equipment = Inventory[EquipmentIndex];
    if (Equipment.MaxCharges <= 0) return false;

    Equipment.MaxCharges--;

    if (Equipment.MaxCharges <= 0)
    {
        // Remove depleted single-use equipment
        RemoveEquipment(EquipmentIndex);
    }

    return true;
}

void USLEquipmentComponent::DrainBattery(int32 EquipmentIndex, float Amount)
{
    if (!Inventory.IsValidIndex(EquipmentIndex)) return;
    Inventory[EquipmentIndex].BatteryLife =
        FMath::Max(0.0f, Inventory[EquipmentIndex].BatteryLife - Amount);
}

bool USLEquipmentComponent::RepairEquipment(int32 EquipmentIndex)
{
    ASLPlayerCharacter* Owner = Cast<ASLPlayerCharacter>(GetOwner());
    if (!Owner || !Owner->bCanRepairEquipment) return false;
    if (!Inventory.IsValidIndex(EquipmentIndex)) return false;

    Inventory[EquipmentIndex].Durability = 100.0f;
    Inventory[EquipmentIndex].BatteryLife = 100.0f;
    Owner->bCanRepairEquipment = false; // Once per run

    UE_LOG(LogSignalLost, Log, TEXT("Equipment repaired (Technician ability used)"));
    return true;
}

// ============================================================================
// FAILURE STATES
// ============================================================================

bool USLEquipmentComponent::IsEquipmentMalfunctioning() const
{
    if (FailureRate <= 0.0f) return false;
    return FMath::FRand() < FailureRate;
}

void USLEquipmentComponent::TickBatteryDrain(float DeltaTime)
{
    if (bFlashlightOn && Inventory.IsValidIndex(ActiveEquipmentIndex))
    {
        FEquipmentData& Active = Inventory[ActiveEquipmentIndex];
        if (Active.Type == EEquipmentType::Flashlight)
        {
            // Drain rate increases slightly with corruption
            float DrainRate = 2.0f; // % per second base
            DrainBattery(ActiveEquipmentIndex, DrainRate * DeltaTime);

            if (Active.BatteryLife <= 0.0f)
            {
                bFlashlightOn = false;
            }
        }
    }
}

void USLEquipmentComponent::TickEquipmentFailures(float DeltaTime)
{
    ASLGameMode* GM = Cast<ASLGameMode>(UGameplayStatics::GetGameMode(GetOwner()));
    if (!GM) return;

    // Failure rate scales with corruption
    float Corruption = GM->CorruptionIndex;
    if (Corruption < 25.0f)
    {
        FailureRate = 0.0f;
    }
    else
    {
        FailureRate = FMath::Lerp(0.0f, 0.15f, (Corruption - 25.0f) / 75.0f);
    }
}

// ============================================================================
// SPECIFIC EQUIPMENT
// ============================================================================

void USLEquipmentComponent::ToggleFlashlight()
{
    bFlashlightOn = !bFlashlightOn;
}

TArray<FVector> USLEquipmentComponent::PulseRadarScan(float Range)
{
    TArray<FVector> Results;
    // Scan for entities and rooms within range
    // Returns positions of detected objects
    // Shows false positives at high corruption
    return Results;
}

void USLEquipmentComponent::FireSignalFlare(FVector Direction)
{
    ConsumeCharge(ActiveEquipmentIndex);
    // Spawn flare projectile that stuns entities for 4 seconds
    UE_LOG(LogSignalLost, Log, TEXT("Signal flare fired"));
}

void USLEquipmentComponent::ThrowNoiseMaker(FVector Direction, float Force)
{
    ConsumeCharge(ActiveEquipmentIndex);
    // Spawn noise maker actor that produces sound for 10 seconds
    UE_LOG(LogSignalLost, Log, TEXT("Noise maker thrown"));
}

void USLEquipmentComponent::ActivateSignalJammer()
{
    ConsumeCharge(ActiveEquipmentIndex);
    // Create 10m dead zone for 30 seconds
    // Blocks entities AND player equipment/radio
    UE_LOG(LogSignalLost, Log, TEXT("Signal jammer activated - 30 second dead zone"));
}

void USLEquipmentComponent::DropGlowStick()
{
    ConsumeCharge(ActiveEquipmentIndex);
    // Spawn permanent glow stick light source at current location
    UE_LOG(LogSignalLost, Log, TEXT("Glow stick dropped"));
}

void USLEquipmentComponent::ActivateEmergencyBeacon()
{
    ConsumeCharge(ActiveEquipmentIndex);

    ASLGameMode* GM = Cast<ASLGameMode>(UGameplayStatics::GetGameMode(GetOwner()));
    if (GM)
    {
        // Reveal airlock direction to all players
        // Also alerts all entities to beacon location
        UE_LOG(LogSignalLost, Warning, TEXT("EMERGENCY BEACON - Airlock revealed, entities alerted!"));
    }
}

void USLEquipmentComponent::StartResync(AActor* Target)
{
    if (!Target) return;
    bIsResyncing = true;
    ResyncTarget = Target;
    ResyncProgress = 0.0f;
}

void USLEquipmentComponent::CancelResync()
{
    bIsResyncing = false;
    ResyncTarget = nullptr;
    ResyncProgress = 0.0f;
    // Kit is still consumed even if interrupted
    ConsumeCharge(ActiveEquipmentIndex);
}

void USLEquipmentComponent::TickResync(float DeltaTime)
{
    if (!ResyncTarget) { CancelResync(); return; }

    // Check distance to target - must maintain close contact
    float Distance = FVector::Dist(GetOwner()->GetActorLocation(), ResyncTarget->GetActorLocation());
    if (Distance > 200.0f)
    {
        CancelResync();
        return;
    }

    ResyncProgress += DeltaTime / ResyncDuration;

    if (ResyncProgress >= 1.0f)
    {
        // Resync complete
        ASLPlayerCharacter* TargetPlayer = Cast<ASLPlayerCharacter>(ResyncTarget);
        if (TargetPlayer)
        {
            if (TargetPlayer->CurrentState == EPlayerState::SignalFade)
            {
                TargetPlayer->Revive(50.0f);
            }
            else
            {
                TargetPlayer->ResetPerceptionDrift();
            }
        }

        ConsumeCharge(ActiveEquipmentIndex);
        bIsResyncing = false;
        ResyncTarget = nullptr;
        ResyncProgress = 0.0f;

        UE_LOG(LogSignalLost, Log, TEXT("Resync complete"));
    }
}

void USLEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(USLEquipmentComponent, Inventory);
    DOREPLIFETIME(USLEquipmentComponent, ActiveEquipmentIndex);
}

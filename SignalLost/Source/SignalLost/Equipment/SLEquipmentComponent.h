#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/SLGameTypes.h"
#include "SLEquipmentComponent.generated.h"

/**
 * Equipment component attached to player characters.
 * Manages inventory, active equipment, battery/charges, durability,
 * and failure states tied to Corruption Index.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SIGNALLOST_API USLEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USLEquipmentComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // ========================================================================
    // INVENTORY
    // ========================================================================

    /** All equipment the player is carrying */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
    TArray<FEquipmentData> Inventory;

    /** Currently active/held equipment index */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
    int32 ActiveEquipmentIndex = -1;

    /** Max inventory slots */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    int32 MaxInventorySlots = 4;

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool AddEquipment(const FEquipmentData& Equipment);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool RemoveEquipment(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void SwitchEquipment(int32 Index);

    UFUNCTION(BlueprintPure, Category = "Equipment")
    FEquipmentData GetActiveEquipment() const;

    UFUNCTION(BlueprintPure, Category = "Equipment")
    bool HasEquipment(EEquipmentType Type) const;

    // ========================================================================
    // USAGE
    // ========================================================================

    /** Use the currently active equipment */
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool UseActiveEquipment();

    /** Consume a charge from equipment (noise makers, glow sticks, etc.) */
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool ConsumeCharge(int32 EquipmentIndex);

    /** Drain battery from equipment */
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void DrainBattery(int32 EquipmentIndex, float Amount);

    /** Repair equipment (Technician ability, once per run) */
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool RepairEquipment(int32 EquipmentIndex);

    // ========================================================================
    // FAILURE STATES
    // ========================================================================

    /** Is equipment affected by corruption-based failures? */
    UFUNCTION(BlueprintPure, Category = "Equipment")
    bool IsEquipmentMalfunctioning() const;

    /** Is equipment jammed by The Broadcast? */
    UFUNCTION(BlueprintPure, Category = "Equipment")
    bool IsJammed() const { return bIsJammed; }

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    bool bIsJammed = false;

    /** Equipment failure rate increases with corruption */
    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    float FailureRate = 0.0f;

    // ========================================================================
    // SPECIFIC EQUIPMENT FUNCTIONS
    // ========================================================================

    // --- Flashlight ---
    UFUNCTION(BlueprintCallable, Category = "Equipment|Flashlight")
    void ToggleFlashlight();

    UPROPERTY(BlueprintReadOnly, Category = "Equipment|Flashlight")
    bool bFlashlightOn = false;

    // --- Scanner/Pulse Radar ---
    UFUNCTION(BlueprintCallable, Category = "Equipment|Scanner")
    TArray<FVector> PulseRadarScan(float Range = 2000.0f);

    // --- Signal Flare ---
    UFUNCTION(BlueprintCallable, Category = "Equipment|Flare")
    void FireSignalFlare(FVector Direction);

    // --- Noise Maker ---
    UFUNCTION(BlueprintCallable, Category = "Equipment|NoiseMaker")
    void ThrowNoiseMaker(FVector Direction, float Force);

    // --- Signal Jammer ---
    UFUNCTION(BlueprintCallable, Category = "Equipment|Jammer")
    void ActivateSignalJammer();

    // --- Glow Stick ---
    UFUNCTION(BlueprintCallable, Category = "Equipment|GlowStick")
    void DropGlowStick();

    // --- Emergency Beacon ---
    UFUNCTION(BlueprintCallable, Category = "Equipment|Beacon")
    void ActivateEmergencyBeacon();

    // --- Resync Kit ---
    UFUNCTION(BlueprintCallable, Category = "Equipment|ResyncKit")
    void StartResync(AActor* Target);

    UFUNCTION(BlueprintCallable, Category = "Equipment|ResyncKit")
    void CancelResync();

    UPROPERTY(BlueprintReadOnly, Category = "Equipment|ResyncKit")
    bool bIsResyncing = false;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment|ResyncKit")
    float ResyncProgress = 0.0f;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void TickBatteryDrain(float DeltaTime);
    void TickEquipmentFailures(float DeltaTime);
    void TickResync(float DeltaTime);

    float EquipmentFailureCheckTimer = 0.0f;

    // Resync state
    AActor* ResyncTarget = nullptr;
    float ResyncDuration = 5.0f;
};

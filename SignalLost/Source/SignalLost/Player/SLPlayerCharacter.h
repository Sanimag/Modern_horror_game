#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/SLGameTypes.h"
#include "SLPlayerCharacter.generated.h"

class USLEquipmentComponent;
class USLPerceptionDriftComponent;
class USLVoiceChatComponent;

/**
 * Player character for Signal Lost.
 * Handles movement, role abilities, health, equipment interaction,
 * perception drift, and signal core carrying.
 */
UCLASS()
class SIGNALLOST_API ASLPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ASLPlayerCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ========================================================================
    // ROLE SYSTEM
    // ========================================================================

    UPROPERTY(ReplicatedUsing=OnRep_PlayerRole, EditAnywhere, BlueprintReadWrite, Category = "Role")
    EPlayerRole CurrentRole = EPlayerRole::None;

    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Role")
    void ServerSetRole(EPlayerRole NewRole);

    UFUNCTION(BlueprintPure, Category = "Role")
    FText GetRoleDisplayName() const;

    // Role-specific passive abilities
    UPROPERTY(BlueprintReadOnly, Category = "Role")
    bool bCanSeeCorruptionHUD = false; // Scanner

    UPROPERTY(BlueprintReadOnly, Category = "Role")
    bool bCanRepairEquipment = true; // Technician (once per run)

    UPROPERTY(BlueprintReadOnly, Category = "Role")
    float CorruptionDamageMultiplier = 1.0f; // Warden: 0.7

    UPROPERTY(BlueprintReadOnly, Category = "Role")
    int32 MaxCarryCores = 1; // Courier: 2

    UPROPERTY(BlueprintReadOnly, Category = "Role")
    float CarrySpeedMultiplier = 1.0f; // Courier: 1.1

    // ========================================================================
    // HEALTH & STATE
    // ========================================================================

    UPROPERTY(ReplicatedUsing=OnRep_PlayerState, BlueprintReadOnly, Category = "Health")
    EPlayerState CurrentState = EPlayerState::Alive;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Health")
    float Health = 100.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Health")
    float MaxHealth = 100.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Health")
    float SignalFadeTimer = 60.0f; // Seconds before permanent death

    UFUNCTION(BlueprintCallable, Category = "Health")
    void TakeDamageCustom(float Amount, EEntityType Source);

    UFUNCTION(BlueprintCallable, Category = "Health")
    void EnterSignalFade();

    UFUNCTION(BlueprintCallable, Category = "Health")
    void Revive(float HealthAmount = 50.0f);

    /** Sacrifice self to reveal all entity positions for 15 seconds */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Health")
    void ServerBroadcastSacrifice();

    // ========================================================================
    // PERCEPTION DRIFT
    // ========================================================================

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Perception")
    float PerceptionDrift = 0.0f; // 0-100, hidden from local player

    UFUNCTION(BlueprintCallable, Category = "Perception")
    void AddPerceptionDrift(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Perception")
    void ResetPerceptionDrift();

    /** Check if this player's perception is compromised (visible to others) */
    UFUNCTION(BlueprintPure, Category = "Perception")
    bool IsPerceptionCompromised() const { return PerceptionDrift > 40.0f; }

    /** Get the flashlight flicker pattern that indicates drift to other players */
    UFUNCTION(BlueprintPure, Category = "Perception")
    float GetDriftFlickerIntensity() const;

    // ========================================================================
    // CARRYING & INTERACTION
    // ========================================================================

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Carrying")
    TArray<FSignalCore> CarriedCores;

    UPROPERTY(BlueprintReadOnly, Category = "Carrying")
    bool bIsCarryingCore = false;

    UFUNCTION(BlueprintCallable, Category = "Carrying")
    bool PickUpCore(const FSignalCore& Core);

    UFUNCTION(BlueprintCallable, Category = "Carrying")
    FSignalCore DropCore(int32 Index = 0);

    UFUNCTION(BlueprintCallable, Category = "Carrying")
    void DropAllCores();

    UFUNCTION(BlueprintPure, Category = "Carrying")
    float GetCurrentMovementSpeedMultiplier() const;

    // ========================================================================
    // EQUIPMENT
    // ========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
    USLEquipmentComponent* EquipmentComponent;

    // ========================================================================
    // MOVEMENT
    // ========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float BaseWalkSpeed = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float CrouchSpeed = 150.0f; // Silent movement

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float SprintSpeed = 600.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsCrouching = false;

    UPROPERTY(BlueprintReadOnly, Category = "Movement")
    bool bIsSprinting = false;

    /** Is the player making noise? (footsteps, sprint, equipment) */
    UFUNCTION(BlueprintPure, Category = "Movement")
    float GetNoiseLevel() const;

    // ========================================================================
    // LOCATION
    // ========================================================================

    UFUNCTION(BlueprintPure, Category = "Location")
    bool IsAtAirlock() const;

    UFUNCTION(BlueprintPure, Category = "Location")
    bool IsIsolated() const; // No teammates within 15m

    UFUNCTION(BlueprintPure, Category = "Location")
    float GetDistanceToNearestTeammate() const;

protected:
    UFUNCTION()
    void OnRep_PlayerRole();

    UFUNCTION()
    void OnRep_PlayerState();

    void ApplyRolePassives();
    void TickPerceptionDrift(float DeltaTime);
    void TickSignalFade(float DeltaTime);
    void UpdateMovementSpeed();

    // Input actions
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartSprint();
    void StopSprint();
    void ToggleCrouch();
    void UseEquipment();
    void InteractWithObject();
    void DropItem();
    void ToggleRadio();
    void OpenWristDevice();
};

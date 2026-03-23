#include "SLPlayerCharacter.h"
#include "Core/SLGameMode.h"
#include "Equipment/SLEquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

ASLPlayerCharacter::ASLPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    EquipmentComponent = CreateDefaultSubobject<USLEquipmentComponent>(TEXT("Equipment"));

    GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
    GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
}

void ASLPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    UpdateMovementSpeed();
}

void ASLPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TickPerceptionDrift(DeltaTime);

    if (CurrentState == EPlayerState::SignalFade)
    {
        TickSignalFade(DeltaTime);
    }

    UpdateMovementSpeed();
}

void ASLPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &ASLPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ASLPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
    PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

    PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ASLPlayerCharacter::StartSprint);
    PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ASLPlayerCharacter::StopSprint);
    PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASLPlayerCharacter::ToggleCrouch);
    PlayerInputComponent->BindAction("UseEquipment", IE_Pressed, this, &ASLPlayerCharacter::UseEquipment);
    PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &ASLPlayerCharacter::InteractWithObject);
    PlayerInputComponent->BindAction("Drop", IE_Pressed, this, &ASLPlayerCharacter::DropItem);
    PlayerInputComponent->BindAction("Radio", IE_Pressed, this, &ASLPlayerCharacter::ToggleRadio);
    PlayerInputComponent->BindAction("WristDevice", IE_Pressed, this, &ASLPlayerCharacter::OpenWristDevice);
}

// ============================================================================
// ROLE SYSTEM
// ============================================================================

void ASLPlayerCharacter::ServerSetRole_Implementation(EPlayerRole NewRole)
{
    CurrentRole = NewRole;
    ApplyRolePassives();
}

void ASLPlayerCharacter::OnRep_PlayerRole()
{
    ApplyRolePassives();
}

void ASLPlayerCharacter::ApplyRolePassives()
{
    // Reset to defaults
    bCanSeeCorruptionHUD = false;
    bCanRepairEquipment = false;
    CorruptionDamageMultiplier = 1.0f;
    MaxCarryCores = 1;
    CarrySpeedMultiplier = 1.0f;

    switch (CurrentRole)
    {
    case EPlayerRole::Scanner:
        bCanSeeCorruptionHUD = true;
        break;

    case EPlayerRole::Technician:
        bCanRepairEquipment = true;
        break;

    case EPlayerRole::Warden:
        CorruptionDamageMultiplier = 0.7f; // 30% less corruption damage
        break;

    case EPlayerRole::Courier:
        MaxCarryCores = 2;
        CarrySpeedMultiplier = 1.1f; // 10% faster while carrying
        break;

    case EPlayerRole::Operator:
        // Operator passives handled via drone and door control systems
        break;

    default:
        break;
    }
}

FText ASLPlayerCharacter::GetRoleDisplayName() const
{
    switch (CurrentRole)
    {
    case EPlayerRole::Scanner:    return FText::FromString(TEXT("Scanner"));
    case EPlayerRole::Technician: return FText::FromString(TEXT("Technician"));
    case EPlayerRole::Warden:     return FText::FromString(TEXT("Warden"));
    case EPlayerRole::Courier:    return FText::FromString(TEXT("Courier"));
    case EPlayerRole::Operator:   return FText::FromString(TEXT("Operator"));
    default:                      return FText::FromString(TEXT("Unassigned"));
    }
}

// ============================================================================
// HEALTH & STATE
// ============================================================================

void ASLPlayerCharacter::TakeDamageCustom(float Amount, EEntityType Source)
{
    if (CurrentState != EPlayerState::Alive) return;

    float AdjustedDamage = Amount * CorruptionDamageMultiplier;
    Health = FMath::Clamp(Health - AdjustedDamage, 0.0f, MaxHealth);

    // Entity encounters increase perception drift
    AddPerceptionDrift(Amount * 0.3f);

    if (Health <= 0.0f)
    {
        EnterSignalFade();
    }

    UE_LOG(LogSignalLost, Log, TEXT("Player took %.1f damage from entity %d. Health: %.1f"),
        AdjustedDamage, (int32)Source, Health);
}

void ASLPlayerCharacter::EnterSignalFade()
{
    CurrentState = EPlayerState::SignalFade;
    SignalFadeTimer = 60.0f;
    DropAllCores();

    UE_LOG(LogSignalLost, Warning, TEXT("Player entered Signal Fade - 60 seconds to revive"));
}

void ASLPlayerCharacter::Revive(float HealthAmount)
{
    if (CurrentState != EPlayerState::SignalFade) return;

    CurrentState = EPlayerState::Alive;
    Health = HealthAmount;
    SignalFadeTimer = 60.0f;

    UE_LOG(LogSignalLost, Log, TEXT("Player revived with %.1f health"), HealthAmount);
}

void ASLPlayerCharacter::ServerBroadcastSacrifice_Implementation()
{
    if (CurrentState != EPlayerState::SignalFade) return;

    CurrentState = EPlayerState::Broadcasting;

    // Reveal all entity positions for 15 seconds
    ASLGameMode* GM = Cast<ASLGameMode>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        // Broadcast entity positions to all players
        UE_LOG(LogSignalLost, Warning, TEXT("Player BROADCAST SACRIFICE - entities revealed for 15 seconds"));
    }

    // After 15 seconds, mark as dead
    FTimerHandle DeathTimer;
    GetWorldTimerManager().SetTimer(DeathTimer, [this]()
    {
        CurrentState = EPlayerState::Dead;
    }, 15.0f, false);
}

void ASLPlayerCharacter::TickSignalFade(float DeltaTime)
{
    SignalFadeTimer -= DeltaTime;
    if (SignalFadeTimer <= 0.0f)
    {
        CurrentState = EPlayerState::Dead;
        UE_LOG(LogSignalLost, Error, TEXT("Player permanent death - Signal Fade expired"));
    }
}

void ASLPlayerCharacter::OnRep_PlayerState()
{
    // Update visual state, ragdoll, etc.
}

// ============================================================================
// PERCEPTION DRIFT
// ============================================================================

void ASLPlayerCharacter::AddPerceptionDrift(float Amount)
{
    PerceptionDrift = FMath::Clamp(PerceptionDrift + Amount, 0.0f, 100.0f);

    // Drift increases with isolation
    if (IsIsolated())
    {
        PerceptionDrift = FMath::Clamp(PerceptionDrift + Amount * 0.5f, 0.0f, 100.0f);
    }
}

void ASLPlayerCharacter::ResetPerceptionDrift()
{
    PerceptionDrift = 0.0f;
}

float ASLPlayerCharacter::GetDriftFlickerIntensity() const
{
    // Flashlight flicker pattern visible to other players
    if (PerceptionDrift < 30.0f) return 0.0f;
    return FMath::Lerp(0.0f, 1.0f, (PerceptionDrift - 30.0f) / 70.0f);
}

void ASLPlayerCharacter::TickPerceptionDrift(float DeltaTime)
{
    // Drift increases passively when isolated
    if (IsIsolated() && CurrentState == EPlayerState::Alive)
    {
        AddPerceptionDrift(DeltaTime * 0.5f);
    }

    // At high drift, client-side visual effects kick in
    // These are handled in Blueprint/material parameters
}

// ============================================================================
// CARRYING
// ============================================================================

bool ASLPlayerCharacter::PickUpCore(const FSignalCore& Core)
{
    if (CarriedCores.Num() >= MaxCarryCores) return false;
    if (CurrentState != EPlayerState::Alive) return false;

    CarriedCores.Add(Core);
    bIsCarryingCore = true;
    UpdateMovementSpeed();

    UE_LOG(LogSignalLost, Log, TEXT("Picked up core worth %d credits"), Core.CreditValue);
    return true;
}

FSignalCore ASLPlayerCharacter::DropCore(int32 Index)
{
    FSignalCore DroppedCore;
    if (CarriedCores.IsValidIndex(Index))
    {
        DroppedCore = CarriedCores[Index];
        CarriedCores.RemoveAt(Index);
        bIsCarryingCore = CarriedCores.Num() > 0;
        UpdateMovementSpeed();
    }
    return DroppedCore;
}

void ASLPlayerCharacter::DropAllCores()
{
    CarriedCores.Empty();
    bIsCarryingCore = false;
    UpdateMovementSpeed();
}

float ASLPlayerCharacter::GetCurrentMovementSpeedMultiplier() const
{
    float Multiplier = 1.0f;

    if (bIsCarryingCore)
    {
        float WeightPenalty = 0.0f;
        for (const FSignalCore& Core : CarriedCores)
        {
            WeightPenalty += Core.Weight * 0.15f; // 15% speed loss per weight unit
        }
        Multiplier -= WeightPenalty;
        Multiplier *= CarrySpeedMultiplier; // Courier bonus
    }

    return FMath::Max(Multiplier, 0.4f); // Never slower than 40% speed
}

// ============================================================================
// MOVEMENT
// ============================================================================

void ASLPlayerCharacter::UpdateMovementSpeed()
{
    float SpeedMultiplier = GetCurrentMovementSpeedMultiplier();

    if (bIsSprinting)
    {
        GetCharacterMovement()->MaxWalkSpeed = SprintSpeed * SpeedMultiplier;
    }
    else
    {
        GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SpeedMultiplier;
    }
}

float ASLPlayerCharacter::GetNoiseLevel() const
{
    if (bIsCrouching) return 0.0f; // Silent
    if (bIsSprinting) return 1.0f; // Maximum noise
    return 0.5f; // Normal walking
}

bool ASLPlayerCharacter::IsAtAirlock() const
{
    ASLGameMode* GM = Cast<ASLGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GM) return false;
    return FVector::Dist(GetActorLocation(), GM->AirlockLocation) < 500.0f;
}

bool ASLPlayerCharacter::IsIsolated() const
{
    return GetDistanceToNearestTeammate() > 1500.0f; // ~15m
}

float ASLPlayerCharacter::GetDistanceToNearestTeammate() const
{
    float NearestDist = MAX_FLT;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APawn* OtherPawn = (*It)->GetPawn();
        if (OtherPawn && OtherPawn != this)
        {
            float Dist = FVector::Dist(GetActorLocation(), OtherPawn->GetActorLocation());
            NearestDist = FMath::Min(NearestDist, Dist);
        }
    }
    return NearestDist;
}

// ============================================================================
// INPUT
// ============================================================================

void ASLPlayerCharacter::MoveForward(float Value)
{
    if (Value == 0.0f) return;
    AddMovementInput(GetActorForwardVector(), Value);
}

void ASLPlayerCharacter::MoveRight(float Value)
{
    if (Value == 0.0f) return;
    AddMovementInput(GetActorRightVector(), Value);
}

void ASLPlayerCharacter::StartSprint()
{
    if (bIsCrouching) return;
    bIsSprinting = true;
    UpdateMovementSpeed();
}

void ASLPlayerCharacter::StopSprint()
{
    bIsSprinting = false;
    UpdateMovementSpeed();
}

void ASLPlayerCharacter::ToggleCrouch()
{
    bIsCrouching = !bIsCrouching;
    if (bIsCrouching)
    {
        Crouch();
        bIsSprinting = false;
    }
    else
    {
        UnCrouch();
    }
    UpdateMovementSpeed();
}

void ASLPlayerCharacter::UseEquipment() { /* Delegate to EquipmentComponent */ }
void ASLPlayerCharacter::InteractWithObject() { /* Interaction trace */ }
void ASLPlayerCharacter::DropItem() { if (bIsCarryingCore) DropCore(0); }
void ASLPlayerCharacter::ToggleRadio() { /* Toggle radio comm */ }
void ASLPlayerCharacter::OpenWristDevice() { /* Open wrist UI */ }

void ASLPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ASLPlayerCharacter, CurrentRole);
    DOREPLIFETIME(ASLPlayerCharacter, CurrentState);
    DOREPLIFETIME(ASLPlayerCharacter, Health);
    DOREPLIFETIME(ASLPlayerCharacter, PerceptionDrift);
    DOREPLIFETIME(ASLPlayerCharacter, CarriedCores);
}

#pragma once

#include "SLEntityBase.h"
#include "SLEntityMimic.generated.h"

/**
 * THE MIMIC (Deceptive Trap)
 * Identical to a loot object — signal core, supply cache, or equipment.
 * Indistinguishable until interacted with.
 * Latches onto player on pickup, deals damage, emits signal attracting entities.
 * If player is alone, drags them into the floor over 10 seconds.
 * Counter: Scanner focused scan (3 sec). Faint vibration visual tell.
 */
UCLASS()
class SIGNALLOST_API ASLEntityMimic : public ASLEntityBase
{
    GENERATED_BODY()

public:
    ASLEntityMimic();

    /** What this Mimic is disguised as */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mimic")
    EEquipmentType DisguiseType;

    /** Damage per second while latched */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mimic")
    float LatchDPS = 10.0f;

    /** Time to drag player underground if alone */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mimic")
    float DragDuration = 10.0f;

    /** Radius of the attracting signal when activated */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mimic")
    float SignalAttractRadius = 5000.0f;

    /** Is this Mimic currently latched onto a player? */
    UPROPERTY(BlueprintReadOnly, Category = "Mimic")
    bool bIsLatched = false;

    /** Player the Mimic is latched onto */
    UPROPERTY(BlueprintReadOnly, Category = "Mimic")
    ASLPlayerCharacter* LatchedPlayer = nullptr;

    /** Is this Mimic disguised (not yet triggered)? */
    UPROPERTY(BlueprintReadOnly, Category = "Mimic")
    bool bIsDisguised = true;

    /** Called when a player tries to interact with the disguised Mimic */
    UFUNCTION(BlueprintCallable, Category = "Mimic")
    void OnPlayerInteract(ASLPlayerCharacter* Player);

    /** Called when a teammate pulls the latched player free */
    UFUNCTION(BlueprintCallable, Category = "Mimic")
    void DetachFromPlayer(ASLPlayerCharacter* Rescuer);

    /** Subtle vibration tell — slightly different frequency than real items */
    UFUNCTION(BlueprintPure, Category = "Mimic")
    float GetDisguiseVibrationOffset() const { return 0.02f; }

protected:
    virtual void UpdateBehavior(float DeltaTime) override;

private:
    void LatchOntoPlayer(ASLPlayerCharacter* Player);
    void DragPlayerDown(float DeltaTime);
    void EmitAttractSignal();

    float DragTimer = 0.0f;
    bool bDragging = false;
};

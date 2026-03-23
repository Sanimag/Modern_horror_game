#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/SLGameTypes.h"
#include "SLEntityBase.generated.h"

class ASLPlayerCharacter;

/**
 * Base class for all Signal Lost entities.
 * Entities are manifestations of corrupted signal data — not alive,
 * but patterns that have learned to interact with physical space.
 */
UCLASS(Abstract)
class SIGNALLOST_API ASLEntityBase : public ACharacter
{
    GENERATED_BODY()

public:
    ASLEntityBase();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entity")
    EEntityType EntityType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entity")
    float DetectionRange = 2000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entity")
    float AttackDamage = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entity")
    float MovementSpeed = 300.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Entity")
    ASLPlayerCharacter* CurrentTarget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entity")
    float PerceptionDriftInflicted = 15.0f; // Drift caused by encounter

    /** Can this entity be stunned by the Warden's flare? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entity")
    bool bCanBeStunned = true;

    UPROPERTY(BlueprintReadOnly, Category = "Entity")
    bool bIsStunned = false;

    UFUNCTION(BlueprintCallable, Category = "Entity")
    virtual void OnPlayerDetected(ASLPlayerCharacter* Player);

    UFUNCTION(BlueprintCallable, Category = "Entity")
    virtual void AttackPlayer(ASLPlayerCharacter* Player);

    UFUNCTION(BlueprintCallable, Category = "Entity")
    virtual void Stun(float Duration);

    UFUNCTION(BlueprintCallable, Category = "Entity")
    virtual void OnCorruptionPhaseChanged(ECorruptionPhase NewPhase);

protected:
    virtual void UpdateBehavior(float DeltaTime) PURE_VIRTUAL(ASLEntityBase::UpdateBehavior, );
    virtual ASLPlayerCharacter* FindTarget();

    float StunTimer = 0.0f;

    UFUNCTION()
    void OnStunEnd();
};

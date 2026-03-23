#pragma once

#include "SLEntityBase.h"
#include "SLEntityEcho.generated.h"

/**
 * THE ECHO (Stalker Type)
 * A humanoid silhouette that mimics crew member outlines.
 * Always at the edge of visibility. Never moves when observed directly.
 * Waits until target is isolated, then lunges from blind spot.
 * Counter: Stay in groups. Check behind you. Scanner radar shows flickering blip.
 */
UCLASS()
class SIGNALLOST_API ASLEntityEcho : public ASLEntityBase
{
    GENERATED_BODY()

public:
    ASLEntityEcho();

    /** Distance the Echo tries to maintain from its target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
    float IdealStalkDistance = 1500.0f;

    /** How close it needs to be to attack */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
    float AttackDistance = 200.0f;

    /** Minimum time stalking before it can attack */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
    float MinStalkDuration = 30.0f;

    /** Is the Echo currently being directly observed? */
    UPROPERTY(BlueprintReadOnly, Category = "Echo")
    bool bIsBeingObserved = false;

    /** Time spent stalking current target */
    UPROPERTY(BlueprintReadOnly, Category = "Echo")
    float StalkTimer = 0.0f;

protected:
    virtual void UpdateBehavior(float DeltaTime) override;

private:
    bool IsInPlayerView(ASLPlayerCharacter* Player) const;
    void MoveCloser(float DeltaTime);
    void AttemptLunge();

    enum class EEchoState : uint8
    {
        Idle,
        Stalking,
        Approaching,
        Lunging
    };

    EEchoState EchoState = EEchoState::Idle;
};

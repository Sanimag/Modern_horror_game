#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/SLGameTypes.h"
#include "SLProgressionManager.generated.h"

/**
 * Save data for persistent progression.
 */
UCLASS()
class SIGNALLOST_API USLSaveData : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    int32 CompanyRank = 1;

    UPROPERTY(SaveGame)
    int32 TotalCreditsEarned = 0;

    UPROPERTY(SaveGame)
    int32 CurrentCredits = 0;

    UPROPERTY(SaveGame)
    int32 TotalMissionsCompleted = 0;

    UPROPERTY(SaveGame)
    int32 TotalMissionsFailed = 0;

    UPROPERTY(SaveGame)
    TMap<EPlayerRole, int32> RoleMasteryXP;

    UPROPERTY(SaveGame)
    TMap<EPlayerRole, int32> RoleMasteryLevel;

    UPROPERTY(SaveGame)
    TArray<FSubmarineUpgrade> SubmarineUpgrades;

    UPROPERTY(SaveGame)
    TArray<FEquipmentData> UnlockedEquipment;

    UPROPERTY(SaveGame)
    TArray<FName> UnlockedCosmetics;

    UPROPERTY(SaveGame)
    TMap<FName, int32> EquipmentUpgradeLevels;
};

/**
 * Progression Manager
 * Handles company rank, credits, role mastery, submarine upgrades,
 * and all persistent progression between runs.
 */
UCLASS()
class SIGNALLOST_API USLProgressionManager : public UObject
{
    GENERATED_BODY()

public:
    USLProgressionManager();

    // ========================================================================
    // SAVE/LOAD
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Progression")
    void SaveProgress();

    UFUNCTION(BlueprintCallable, Category = "Progression")
    void LoadProgress();

    UPROPERTY(BlueprintReadOnly, Category = "Progression")
    USLSaveData* SaveData = nullptr;

    // ========================================================================
    // COMPANY RANK
    // ========================================================================

    UFUNCTION(BlueprintPure, Category = "Rank")
    int32 GetCompanyRank() const;

    UFUNCTION(BlueprintPure, Category = "Rank")
    int32 GetCreditsForNextRank() const;

    UFUNCTION(BlueprintPure, Category = "Rank")
    float GetRankProgress() const;

    /** Get max station threat rating unlocked at current rank */
    UFUNCTION(BlueprintPure, Category = "Rank")
    int32 GetMaxUnlockedThreatRating() const;

    // ========================================================================
    // CREDITS
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Credits")
    void AddCredits(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Credits")
    bool SpendCredits(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Credits")
    int32 GetCurrentCredits() const;

    // ========================================================================
    // ROLE MASTERY
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Mastery")
    void AddRoleMasteryXP(EPlayerRole Role, int32 XP);

    UFUNCTION(BlueprintPure, Category = "Mastery")
    int32 GetRoleMasteryLevel(EPlayerRole Role) const;

    UFUNCTION(BlueprintPure, Category = "Mastery")
    float GetRoleMasteryProgress(EPlayerRole Role) const;

    /** Get mastery perks unlocked for a role */
    UFUNCTION(BlueprintPure, Category = "Mastery")
    TArray<FText> GetUnlockedMasteryPerks(EPlayerRole Role) const;

    // ========================================================================
    // SUBMARINE UPGRADES
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Submarine")
    bool PurchaseSubUpgrade(FName UpgradeID);

    UFUNCTION(BlueprintPure, Category = "Submarine")
    int32 GetSubUpgradeLevel(FName UpgradeID) const;

    UFUNCTION(BlueprintPure, Category = "Submarine")
    TArray<FSubmarineUpgrade> GetAvailableSubUpgrades() const;

    // ========================================================================
    // EQUIPMENT SHOP
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool PurchaseEquipment(EEquipmentType Type);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    bool UpgradeEquipment(EEquipmentType Type);

    UFUNCTION(BlueprintPure, Category = "Equipment")
    int32 GetEquipmentUpgradeLevel(EEquipmentType Type) const;

    // ========================================================================
    // MISSION DEBRIEF
    // ========================================================================

    /** Process end-of-mission rewards */
    UFUNCTION(BlueprintCallable, Category = "Debrief")
    int32 ProcessMissionRewards(const TArray<FSignalCore>& ExtractedCores,
        float PayoutMultiplier, EPlayerRole PlayedRole, bool bMissionSuccess);

private:
    static constexpr int32 BASE_CREDITS_PER_RANK = 1000;
    static constexpr float RANK_SCALING = 1.5f;
    static constexpr int32 MASTERY_XP_PER_LEVEL = 500;
    static constexpr int32 MAX_MASTERY_LEVEL = 10;

    void CheckRankUp();

    FString SaveSlotName = TEXT("SignalLostSave");
    int32 SaveUserIndex = 0;
};

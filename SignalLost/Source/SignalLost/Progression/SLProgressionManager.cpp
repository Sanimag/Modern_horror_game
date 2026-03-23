#include "SLProgressionManager.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

USLProgressionManager::USLProgressionManager()
{
    SaveData = NewObject<USLSaveData>();
}

// ============================================================================
// SAVE / LOAD
// ============================================================================

void USLProgressionManager::SaveProgress()
{
    if (SaveData)
    {
        UGameplayStatics::SaveGameToSlot(SaveData, SaveSlotName, SaveUserIndex);
        UE_LOG(LogSignalLost, Log, TEXT("Progress saved. Rank: %d, Credits: %d"),
            SaveData->CompanyRank, SaveData->CurrentCredits);
    }
}

void USLProgressionManager::LoadProgress()
{
    USLSaveData* LoadedData = Cast<USLSaveData>(
        UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));

    if (LoadedData)
    {
        SaveData = LoadedData;
        UE_LOG(LogSignalLost, Log, TEXT("Progress loaded. Rank: %d, Credits: %d"),
            SaveData->CompanyRank, SaveData->CurrentCredits);
    }
    else
    {
        SaveData = NewObject<USLSaveData>();
        UE_LOG(LogSignalLost, Log, TEXT("No save data found. Starting fresh."));
    }
}

// ============================================================================
// COMPANY RANK
// ============================================================================

int32 USLProgressionManager::GetCompanyRank() const
{
    return SaveData ? SaveData->CompanyRank : 1;
}

int32 USLProgressionManager::GetCreditsForNextRank() const
{
    if (!SaveData) return BASE_CREDITS_PER_RANK;
    return FMath::RoundToInt(BASE_CREDITS_PER_RANK *
        FMath::Pow(RANK_SCALING, SaveData->CompanyRank - 1));
}

float USLProgressionManager::GetRankProgress() const
{
    if (!SaveData) return 0.0f;
    int32 CreditsNeeded = GetCreditsForNextRank();
    return (float)SaveData->TotalCreditsEarned / (float)CreditsNeeded;
}

int32 USLProgressionManager::GetMaxUnlockedThreatRating() const
{
    if (!SaveData) return 1;
    // Unlock higher threat ratings every 3 ranks
    return FMath::Clamp(1 + (SaveData->CompanyRank / 3), 1, 5);
}

void USLProgressionManager::CheckRankUp()
{
    if (!SaveData) return;

    int32 CreditsNeeded = GetCreditsForNextRank();
    while (SaveData->TotalCreditsEarned >= CreditsNeeded)
    {
        SaveData->TotalCreditsEarned -= CreditsNeeded;
        SaveData->CompanyRank++;
        CreditsNeeded = GetCreditsForNextRank();

        UE_LOG(LogSignalLost, Warning, TEXT("RANK UP! New company rank: %d"),
            SaveData->CompanyRank);
    }
}

// ============================================================================
// CREDITS
// ============================================================================

void USLProgressionManager::AddCredits(int32 Amount)
{
    if (!SaveData || Amount <= 0) return;
    SaveData->CurrentCredits += Amount;
    SaveData->TotalCreditsEarned += Amount;
    CheckRankUp();
}

bool USLProgressionManager::SpendCredits(int32 Amount)
{
    if (!SaveData || Amount <= 0) return false;
    if (SaveData->CurrentCredits < Amount) return false;

    SaveData->CurrentCredits -= Amount;
    return true;
}

int32 USLProgressionManager::GetCurrentCredits() const
{
    return SaveData ? SaveData->CurrentCredits : 0;
}

// ============================================================================
// ROLE MASTERY
// ============================================================================

void USLProgressionManager::AddRoleMasteryXP(EPlayerRole Role, int32 XP)
{
    if (!SaveData || Role == EPlayerRole::None) return;

    int32& CurrentXP = SaveData->RoleMasteryXP.FindOrAdd(Role, 0);
    int32& CurrentLevel = SaveData->RoleMasteryLevel.FindOrAdd(Role, 0);

    CurrentXP += XP;

    while (CurrentXP >= MASTERY_XP_PER_LEVEL && CurrentLevel < MAX_MASTERY_LEVEL)
    {
        CurrentXP -= MASTERY_XP_PER_LEVEL;
        CurrentLevel++;
        UE_LOG(LogSignalLost, Log, TEXT("Role mastery level up! Role: %d, Level: %d"),
            (int32)Role, CurrentLevel);
    }
}

int32 USLProgressionManager::GetRoleMasteryLevel(EPlayerRole Role) const
{
    if (!SaveData) return 0;
    const int32* Level = SaveData->RoleMasteryLevel.Find(Role);
    return Level ? *Level : 0;
}

float USLProgressionManager::GetRoleMasteryProgress(EPlayerRole Role) const
{
    if (!SaveData) return 0.0f;
    const int32* XP = SaveData->RoleMasteryXP.Find(Role);
    return XP ? (float)(*XP) / (float)MASTERY_XP_PER_LEVEL : 0.0f;
}

TArray<FText> USLProgressionManager::GetUnlockedMasteryPerks(EPlayerRole Role) const
{
    TArray<FText> Perks;
    int32 Level = GetRoleMasteryLevel(Role);

    switch (Role)
    {
    case EPlayerRole::Scanner:
        if (Level >= 3) Perks.Add(FText::FromString(TEXT("Extended radar range (+30%)")));
        if (Level >= 5) Perks.Add(FText::FromString(TEXT("Passive Mimic detection (subtle visual cue)")));
        if (Level >= 8) Perks.Add(FText::FromString(TEXT("Corruption phase prediction (10s warning)")));
        break;

    case EPlayerRole::Technician:
        if (Level >= 3) Perks.Add(FText::FromString(TEXT("Faster extraction (-20% time)")));
        if (Level >= 5) Perks.Add(FText::FromString(TEXT("Second equipment repair per run")));
        if (Level >= 8) Perks.Add(FText::FromString(TEXT("Core integrity preservation (+15%)")));
        break;

    case EPlayerRole::Warden:
        if (Level >= 3) Perks.Add(FText::FromString(TEXT("Extra flare charge (+1)")));
        if (Level >= 5) Perks.Add(FText::FromString(TEXT("Corruption resistance (+40% total)")));
        if (Level >= 8) Perks.Add(FText::FromString(TEXT("Flare stun duration extended (+2 sec)")));
        break;

    case EPlayerRole::Courier:
        if (Level >= 3) Perks.Add(FText::FromString(TEXT("Carry speed bonus (+15% total)")));
        if (Level >= 5) Perks.Add(FText::FromString(TEXT("Third core carry slot")));
        if (Level >= 8) Perks.Add(FText::FromString(TEXT("Core protection (dropped cores retain integrity)")));
        break;

    case EPlayerRole::Operator:
        if (Level >= 3) Perks.Add(FText::FromString(TEXT("Extended drone range (+50%)")));
        if (Level >= 5) Perks.Add(FText::FromString(TEXT("Drone noise maker ability")));
        if (Level >= 8) Perks.Add(FText::FromString(TEXT("Remote equipment activation through doors")));
        break;

    default:
        break;
    }

    return Perks;
}

// ============================================================================
// SUBMARINE UPGRADES
// ============================================================================

bool USLProgressionManager::PurchaseSubUpgrade(FName UpgradeID)
{
    if (!SaveData) return false;

    for (FSubmarineUpgrade& Upgrade : SaveData->SubmarineUpgrades)
    {
        if (Upgrade.UpgradeID == UpgradeID && Upgrade.CurrentLevel < Upgrade.MaxLevel)
        {
            int32 Cost = Upgrade.Cost * (Upgrade.CurrentLevel + 1);
            if (SpendCredits(Cost))
            {
                Upgrade.CurrentLevel++;
                SaveProgress();
                return true;
            }
            return false;
        }
    }
    return false;
}

int32 USLProgressionManager::GetSubUpgradeLevel(FName UpgradeID) const
{
    if (!SaveData) return 0;
    for (const FSubmarineUpgrade& Upgrade : SaveData->SubmarineUpgrades)
    {
        if (Upgrade.UpgradeID == UpgradeID) return Upgrade.CurrentLevel;
    }
    return 0;
}

TArray<FSubmarineUpgrade> USLProgressionManager::GetAvailableSubUpgrades() const
{
    if (!SaveData) return TArray<FSubmarineUpgrade>();
    return SaveData->SubmarineUpgrades;
}

// ============================================================================
// EQUIPMENT SHOP
// ============================================================================

bool USLProgressionManager::PurchaseEquipment(EEquipmentType Type)
{
    // Equipment purchasing handled by the requisition terminal UI
    return false;
}

bool USLProgressionManager::UpgradeEquipment(EEquipmentType Type)
{
    if (!SaveData) return false;

    FName TypeName = FName(*FString::Printf(TEXT("Equip_%d"), (int32)Type));
    int32& Level = SaveData->EquipmentUpgradeLevels.FindOrAdd(TypeName, 0);

    if (Level >= 3) return false; // Max upgrade level

    int32 UpgradeCost = 200 * (Level + 1);
    if (SpendCredits(UpgradeCost))
    {
        Level++;
        SaveProgress();
        return true;
    }
    return false;
}

int32 USLProgressionManager::GetEquipmentUpgradeLevel(EEquipmentType Type) const
{
    if (!SaveData) return 0;
    FName TypeName = FName(*FString::Printf(TEXT("Equip_%d"), (int32)Type));
    const int32* Level = SaveData->EquipmentUpgradeLevels.Find(TypeName);
    return Level ? *Level : 0;
}

// ============================================================================
// MISSION DEBRIEF
// ============================================================================

int32 USLProgressionManager::ProcessMissionRewards(const TArray<FSignalCore>& ExtractedCores,
    float PayoutMultiplier, EPlayerRole PlayedRole, bool bMissionSuccess)
{
    int32 TotalPayout = 0;

    for (const FSignalCore& Core : ExtractedCores)
    {
        float IntegrityBonus = Core.Integrity / 100.0f;
        TotalPayout += FMath::RoundToInt(Core.CreditValue * IntegrityBonus * PayoutMultiplier);
    }

    // Failure penalty: 50% payout reduction
    if (!bMissionSuccess)
    {
        TotalPayout = FMath::RoundToInt(TotalPayout * 0.5f);
    }

    AddCredits(TotalPayout);

    // Role mastery XP
    int32 BaseXP = bMissionSuccess ? 100 : 30;
    BaseXP += ExtractedCores.Num() * 20;
    AddRoleMasteryXP(PlayedRole, BaseXP);

    // Update mission stats
    if (SaveData)
    {
        if (bMissionSuccess) SaveData->TotalMissionsCompleted++;
        else SaveData->TotalMissionsFailed++;
    }

    SaveProgress();

    UE_LOG(LogSignalLost, Log, TEXT("Mission debrief: %d cores, %d credits, %d mastery XP"),
        ExtractedCores.Num(), TotalPayout, BaseXP);

    return TotalPayout;
}

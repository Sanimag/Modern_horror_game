#include "SLMissionManager.h"
#include "SignalLost.h"

USLMissionManager::USLMissionManager()
{
}

// ============================================================================
// CONTRACT MANAGEMENT
// ============================================================================

TArray<FMissionContract> USLMissionManager::GenerateAvailableContracts(int32 CompanyRank)
{
    AvailableContracts.Empty();

    // Always offer standard salvage
    AvailableContracts.Add(CreateContract(EMissionType::StandardSalvage,
        FMath::Clamp(CompanyRank / 2, 1, 5), 1.0f));

    // Rank 2+: Data Recovery
    if (CompanyRank >= 2)
    {
        AvailableContracts.Add(CreateContract(EMissionType::DataRecovery,
            FMath::Clamp(CompanyRank / 2 + 1, 2, 5), 1.3f));
    }

    // Rank 4+: Rescue Ops
    if (CompanyRank >= 4)
    {
        AvailableContracts.Add(CreateContract(EMissionType::RescueOp,
            FMath::Clamp(CompanyRank / 3 + 1, 2, 5), 1.5f));
    }

    // Rank 6+: Purge Contracts
    if (CompanyRank >= 6)
    {
        AvailableContracts.Add(CreateContract(EMissionType::PurgeContract,
            FMath::Clamp(CompanyRank / 3 + 2, 3, 5), 2.0f));
    }

    // Rank 8+: Black Box
    if (CompanyRank >= 8)
    {
        AvailableContracts.Add(CreateContract(EMissionType::BlackBox, 5, 3.0f));
    }

    UE_LOG(LogSignalLost, Log, TEXT("Generated %d contracts for rank %d"),
        AvailableContracts.Num(), CompanyRank);

    return AvailableContracts;
}

FMissionContract USLMissionManager::GetWeeklyContract() const
{
    // Generate a deterministic weekly seed based on current date
    FDateTime Now = FDateTime::Now();
    int32 WeekSeed = Now.GetYear() * 100 + (Now.GetDayOfYear() / 7);

    FMissionContract Weekly;
    Weekly.ContractID = FName(TEXT("Weekly"));
    Weekly.MissionType = EMissionType::StandardSalvage;
    Weekly.ThreatRating = 3;
    Weekly.PayoutMultiplier = 2.0f;
    Weekly.Description = FText::FromString(TEXT("Weekly Featured Station - Shared leaderboard"));

    return Weekly;
}

void USLMissionManager::SelectContract(const FMissionContract& Contract)
{
    SelectedContract = Contract;
    UE_LOG(LogSignalLost, Log, TEXT("Contract selected: %s (Threat: %d, Payout: %.1fx)"),
        *SelectedContract.ContractID.ToString(), SelectedContract.ThreatRating,
        SelectedContract.PayoutMultiplier);
}

FMissionContract USLMissionManager::CreateContract(EMissionType Type, int32 ThreatRating, float PayoutMult)
{
    FMissionContract Contract;
    Contract.ContractID = FName(*FString::Printf(TEXT("Contract_%d_%d"),
        (int32)Type, FMath::Rand()));
    Contract.MissionType = Type;
    Contract.ThreatRating = ThreatRating;
    Contract.PayoutMultiplier = PayoutMult;
    Contract.BaseCorruption = (ThreatRating - 1) * 5.0f; // Higher threat = starting corruption
    Contract.MinDepth = ThreatRating;

    // Set descriptions
    switch (Type)
    {
    case EMissionType::StandardSalvage:
        Contract.Description = FText::FromString(
            TEXT("Extract signal cores from the station. More cores = more credits = more danger."));
        break;
    case EMissionType::DataRecovery:
        Contract.Description = FText::FromString(
            TEXT("Download files from 3 terminals deep in the station. Terminals are in high-Corruption zones."));
        break;
    case EMissionType::RescueOp:
        Contract.Description = FText::FromString(
            TEXT("Find and extract a survivor. Warning: survivor has high Perception Drift."));
        break;
    case EMissionType::PurgeContract:
        Contract.Description = FText::FromString(
            TEXT("Destroy a specific entity. Special equipment required. Extremely dangerous."));
        break;
    case EMissionType::BlackBox:
        Contract.Description = FText::FromString(
            TEXT("Retrieve the station's flight recorder from the deepest point. One-way trip."));
        break;
    }

    // Guaranteed entities based on threat
    if (ThreatRating >= 3) Contract.GuaranteedEntities.Add(EEntityType::Echo);
    if (ThreatRating >= 4) Contract.GuaranteedEntities.Add(EEntityType::Architect);
    if (Type == EMissionType::PurgeContract) Contract.GuaranteedEntities.Add(EEntityType::Resonant);

    return Contract;
}

// ============================================================================
// DEPLOYMENT
// ============================================================================

void USLMissionManager::BeginDeployment()
{
    bIsDeploying = true;
    PressurizationProgress = 0.0f;
    UE_LOG(LogSignalLost, Log, TEXT("Submarine deploying to station..."));
}

void USLMissionManager::OnDockingComplete()
{
    UE_LOG(LogSignalLost, Log, TEXT("Docking complete. Beginning pressurization."));
    BeginPressurization();
}

void USLMissionManager::BeginPressurization()
{
    PressurizationProgress = 0.0f;
    // 30-second pressurization sequence builds tension
    UE_LOG(LogSignalLost, Log, TEXT("Pressurization sequence started (30 seconds)"));
}

// ============================================================================
// CORE EXTRACTION
// ============================================================================

void USLMissionManager::BeginCoreExtraction(const FSignalCore& Core)
{
    if (bIsExtracting) return;

    bIsExtracting = true;
    ExtractionProgress = 0.0f;
    ExtractionPhase = 0;
    ExtractingCore = Core;

    UE_LOG(LogSignalLost, Log, TEXT("Beginning core extraction: %s (Value: %d, Corruption cost: %.1f%%)"),
        *Core.CoreID.ToString(), Core.CreditValue, Core.CorruptionCost);
}

void USLMissionManager::UpdateCoreExtraction(float DeltaTime)
{
    if (!bIsExtracting) return;

    float Duration = GetExtractionDuration(false); // TODO: check for Technician accelerator
    ExtractionProgress += DeltaTime / Duration;

    // Update extraction phase
    if (ExtractionProgress < 0.33f)
        ExtractionPhase = 0; // Scanning
    else if (ExtractionProgress < 0.66f)
        ExtractionPhase = 1; // Disconnecting power couplings
    else
        ExtractionPhase = 2; // Physical pull

    if (ExtractionProgress >= 1.0f)
    {
        // Extraction complete
        bIsExtracting = false;
        ExtractedCoresThisMission.Add(ExtractingCore);

        UE_LOG(LogSignalLost, Log, TEXT("Core extracted! Total this mission: %d"),
            ExtractedCoresThisMission.Num());

        // Corruption increase is handled by the game mode
    }
}

void USLMissionManager::CancelCoreExtraction()
{
    bIsExtracting = false;
    ExtractionProgress = 0.0f;
    ExtractionPhase = 0;
    UE_LOG(LogSignalLost, Warning, TEXT("Core extraction cancelled"));
}

float USLMissionManager::GetExtractionDuration(bool bHasAccelerator) const
{
    return bHasAccelerator ? AcceleratedExtractionTime : BaseExtractionTime;
}

// ============================================================================
// AIRLOCK
// ============================================================================

bool USLMissionManager::CanExtract() const
{
    return GetUnaccountedPlayers().Num() == 0;
}

TArray<FName> USLMissionManager::GetUnaccountedPlayers() const
{
    TArray<FName> Unaccounted;
    // Check for players not at airlock and not confirmed dead
    return Unaccounted;
}

void USLMissionManager::ForceExtraction()
{
    UE_LOG(LogSignalLost, Warning, TEXT("FORCE EXTRACTION - leaving players behind!"));
    BeginAirlockSeal();
}

void USLMissionManager::BeginAirlockSeal()
{
    AirlockSealProgress = 0.0f;
    UE_LOG(LogSignalLost, Log, TEXT("Airlock seal initiated"));
}

// ============================================================================
// DEBRIEF
// ============================================================================

void USLMissionManager::ProcessDebrief()
{
    MissionCreditsEarned = 0;

    for (const FSignalCore& Core : ExtractedCoresThisMission)
    {
        float IntegrityBonus = Core.Integrity / 100.0f;
        MissionCreditsEarned += FMath::RoundToInt(
            Core.CreditValue * IntegrityBonus * SelectedContract.PayoutMultiplier);
    }

    if (!bMissionSuccess)
    {
        MissionCreditsEarned = FMath::RoundToInt(MissionCreditsEarned * 0.5f);
    }

    UE_LOG(LogSignalLost, Log, TEXT("Mission debrief: %d cores extracted, %d credits earned, success: %s"),
        ExtractedCoresThisMission.Num(), MissionCreditsEarned,
        bMissionSuccess ? TEXT("YES") : TEXT("NO"));
}

// ============================================================================
// DYNAMIC EVENTS
// ============================================================================

void USLMissionManager::CheckDynamicEvents(float CorruptionPercent)
{
    // Random events not tied to corruption
    float Roll = FMath::FRand();

    if (Roll < 0.15f)
    {
        // Hull breach - flood a section
        ActiveEvents.Add(FName("HullBreach"));
        UE_LOG(LogSignalLost, Warning, TEXT("DYNAMIC EVENT: Hull breach!"));
    }
    else if (Roll < 0.25f)
    {
        // Power surge - illuminate station temporarily
        ActiveEvents.Add(FName("PowerSurge"));
        UE_LOG(LogSignalLost, Warning, TEXT("DYNAMIC EVENT: Power surge - station illuminated!"));
    }
    else if (Roll < 0.35f)
    {
        // Distress signal from "other crew"
        ActiveEvents.Add(FName("DistressSignal"));
        UE_LOG(LogSignalLost, Warning, TEXT("DYNAMIC EVENT: Distress signal detected"));
    }
    else if (Roll < 0.45f)
    {
        // Equipment cache from failed crew
        ActiveEvents.Add(FName("EquipmentCache"));
        UE_LOG(LogSignalLost, Log, TEXT("DYNAMIC EVENT: Previous crew equipment cache found"));
    }
}

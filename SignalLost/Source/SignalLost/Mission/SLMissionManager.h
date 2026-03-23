#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/SLGameTypes.h"
#include "SLMissionManager.generated.h"

class ASLGameMode;
class ASLStationGenerator;
class USLProgressionManager;

/**
 * Mission Manager
 * Handles the full mission lifecycle: contract selection, deployment,
 * core extraction workflow, extraction conditions, and debrief.
 */
UCLASS(BlueprintType)
class SIGNALLOST_API USLMissionManager : public UObject
{
    GENERATED_BODY()

public:
    USLMissionManager();

    // ========================================================================
    // CONTRACT MANAGEMENT
    // ========================================================================

    /** Generate available contracts based on company rank */
    UFUNCTION(BlueprintCallable, Category = "Contracts")
    TArray<FMissionContract> GenerateAvailableContracts(int32 CompanyRank);

    /** Get the weekly featured contract */
    UFUNCTION(BlueprintCallable, Category = "Contracts")
    FMissionContract GetWeeklyContract() const;

    /** Select a contract for the current mission */
    UFUNCTION(BlueprintCallable, Category = "Contracts")
    void SelectContract(const FMissionContract& Contract);

    UPROPERTY(BlueprintReadOnly, Category = "Contracts")
    FMissionContract SelectedContract;

    UPROPERTY(BlueprintReadOnly, Category = "Contracts")
    TArray<FMissionContract> AvailableContracts;

    // ========================================================================
    // DEPLOYMENT
    // ========================================================================

    /** Start the deployment sequence (submarine descent) */
    UFUNCTION(BlueprintCallable, Category = "Deployment")
    void BeginDeployment();

    /** Called when the submarine docking is complete */
    UFUNCTION(BlueprintCallable, Category = "Deployment")
    void OnDockingComplete();

    /** Start the 30-second pressurization sequence */
    UFUNCTION(BlueprintCallable, Category = "Deployment")
    void BeginPressurization();

    UPROPERTY(BlueprintReadOnly, Category = "Deployment")
    float PressurizationProgress = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Deployment")
    bool bIsDeploying = false;

    // ========================================================================
    // CORE EXTRACTION
    // ========================================================================

    /** Begin extraction of a signal core (multi-step process) */
    UFUNCTION(BlueprintCallable, Category = "Extraction")
    void BeginCoreExtraction(const FSignalCore& Core);

    /** Update extraction progress */
    UFUNCTION(BlueprintCallable, Category = "Extraction")
    void UpdateCoreExtraction(float DeltaTime);

    /** Cancel an in-progress extraction */
    UFUNCTION(BlueprintCallable, Category = "Extraction")
    void CancelCoreExtraction();

    UPROPERTY(BlueprintReadOnly, Category = "Extraction")
    bool bIsExtracting = false;

    UPROPERTY(BlueprintReadOnly, Category = "Extraction")
    float ExtractionProgress = 0.0f; // 0-1

    /** Current extraction phase (scan, disconnect, pull) */
    UPROPERTY(BlueprintReadOnly, Category = "Extraction")
    int32 ExtractionPhase = 0; // 0=scan, 1=disconnect, 2=pull

    UPROPERTY(BlueprintReadOnly, Category = "Extraction")
    FSignalCore ExtractingCore;

    // ========================================================================
    // AIRLOCK & EXIT
    // ========================================================================

    /** Check if extraction conditions are met */
    UFUNCTION(BlueprintPure, Category = "Airlock")
    bool CanExtract() const;

    /** Get list of unaccounted players */
    UFUNCTION(BlueprintPure, Category = "Airlock")
    TArray<FName> GetUnaccountedPlayers() const;

    /** Force extraction (leave unaccounted players behind) */
    UFUNCTION(BlueprintCallable, Category = "Airlock")
    void ForceExtraction();

    /** Begin airlock seal sequence */
    UFUNCTION(BlueprintCallable, Category = "Airlock")
    void BeginAirlockSeal();

    UPROPERTY(BlueprintReadOnly, Category = "Airlock")
    float AirlockSealProgress = 0.0f;

    // ========================================================================
    // DEBRIEF
    // ========================================================================

    /** Process end-of-mission and calculate rewards */
    UFUNCTION(BlueprintCallable, Category = "Debrief")
    void ProcessDebrief();

    UPROPERTY(BlueprintReadOnly, Category = "Debrief")
    int32 MissionCreditsEarned = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Debrief")
    TArray<FSignalCore> ExtractedCoresThisMission;

    UPROPERTY(BlueprintReadOnly, Category = "Debrief")
    bool bMissionSuccess = false;

    // ========================================================================
    // DYNAMIC EVENTS
    // ========================================================================

    /** Roll for random dynamic events during the run */
    UFUNCTION(BlueprintCallable, Category = "Events")
    void CheckDynamicEvents(float CorruptionPercent);

    /** Event types that can occur */
    UPROPERTY(BlueprintReadOnly, Category = "Events")
    TArray<FName> ActiveEvents;

private:
    FMissionContract CreateContract(EMissionType Type, int32 ThreatRating, float PayoutMult);
    float GetExtractionDuration(bool bHasAccelerator) const;

    // Extraction timing
    float BaseExtractionTime = 15.0f; // seconds for full extraction
    float AcceleratedExtractionTime = 7.5f;

    // Dynamic event cooldown
    float EventCheckTimer = 0.0f;
    float EventCheckInterval = 60.0f;
};

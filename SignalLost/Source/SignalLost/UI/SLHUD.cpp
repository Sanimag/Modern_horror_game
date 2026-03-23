#include "SLHUD.h"
#include "Player/SLPlayerCharacter.h"
#include "Core/SLGameMode.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "SignalLost.h"

ASLHUD::ASLHUD()
{
}

void ASLHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!OwnerCharacter)
    {
        OwnerCharacter = Cast<ASLPlayerCharacter>(GetOwningPawn());
    }
    if (!OwnerCharacter) return;

    ASLGameMode* GM = Cast<ASLGameMode>(UGameplayStatics::GetGameMode(this));

    // Always draw: health vignette
    DrawHealthVignette(OwnerCharacter->Health, OwnerCharacter->MaxHealth);

    // Always draw: airlock compass
    if (GM)
    {
        FVector ToAirlock = GM->AirlockLocation - OwnerCharacter->GetActorLocation();
        DrawAirlockCompass(ToAirlock);
    }

    // Conditional: Corruption indicator (Scanner only)
    if (OwnerCharacter->bCanSeeCorruptionHUD && GM)
    {
        DrawCorruptionIndicator(GM->CorruptionIndex, GM->CurrentPhase);
    }

    // Conditional: Signal Fade timer
    if (OwnerCharacter->CurrentState == EPlayerState::SignalFade)
    {
        DrawSignalFadeTimer(OwnerCharacter->SignalFadeTimer);
    }

    // Conditional: Cascade countdown
    if (GM && GM->bCascadeActive)
    {
        DrawCascadeCountdown(GM->CascadeCountdown);
    }

    // Perception drift effects (player doesn't know they're affected)
    if (OwnerCharacter->PerceptionDrift > 20.0f)
    {
        DrawPerceptionDriftEffects(OwnerCharacter->PerceptionDrift);
    }

    // Teammate indicators
    DrawTeammateIndicators();

    // Wrist device overlay
    if (bWristDeviceOpen)
    {
        DrawWristDevice();
    }

    // Timed effects
    if (FrequencyBurnTimer > 0.0f)
    {
        FrequencyBurnTimer -= GetWorld()->GetDeltaSeconds();
        // Apply screen distortion effect
    }
}

void ASLHUD::DrawHealthVignette(float Health, float MaxHealth)
{
    float HealthPercent = Health / MaxHealth;

    // Vignette intensity increases as health drops
    // At 100%: no vignette. At 25%: heavy dark vignette. At 10%: pulsing red.
    if (HealthPercent >= 0.8f) return;

    float Intensity = 1.0f - HealthPercent;
    FLinearColor VignetteColor = FLinearColor::Black;

    if (HealthPercent < 0.25f)
    {
        // Pulsing red at critical health
        float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 3.0f) * 0.5f + 0.5f;
        VignetteColor = FLinearColor(0.5f * Pulse, 0.0f, 0.0f, Intensity * 0.6f);
    }
    else
    {
        VignetteColor = FLinearColor(0.0f, 0.0f, 0.0f, Intensity * 0.4f);
    }

    // Draw vignette as screen-space overlay
    // In production, this would be a post-process material parameter
}

void ASLHUD::DrawCorruptionIndicator(float Corruption, ECorruptionPhase Phase)
{
    if (!Canvas) return;

    FString PhaseText;
    FLinearColor PhaseColor;

    switch (Phase)
    {
    case ECorruptionPhase::Quiet:
        PhaseText = TEXT("QUIET"); PhaseColor = FLinearColor::Green; break;
    case ECorruptionPhase::Stirring:
        PhaseText = TEXT("STIRRING"); PhaseColor = FLinearColor::Yellow; break;
    case ECorruptionPhase::Active:
        PhaseText = TEXT("ACTIVE"); PhaseColor = FLinearColor(1.0f, 0.5f, 0.0f); break;
    case ECorruptionPhase::Critical:
        PhaseText = TEXT("CRITICAL"); PhaseColor = FLinearColor::Red; break;
    case ECorruptionPhase::Cascade:
        PhaseText = TEXT("CASCADE"); PhaseColor = FLinearColor(1.0f, 0.0f, 0.0f); break;
    }

    // Draw in top-right corner, small text
    FString CorruptionText = FString::Printf(TEXT("%.0f%% %s"), Corruption, *PhaseText);
    float X = Canvas->SizeX - 200.0f;
    float Y = 30.0f;

    Canvas->SetDrawColor(PhaseColor.ToFColor(true));
    Canvas->DrawText(GEngine->GetSmallFont(), CorruptionText, X, Y);
}

void ASLHUD::DrawSignalFadeTimer(float TimeRemaining)
{
    if (!Canvas) return;

    FString TimerText = FString::Printf(TEXT("SIGNAL FADING: %.0f"), TimeRemaining);
    float X = Canvas->SizeX * 0.5f - 100.0f;
    float Y = Canvas->SizeY * 0.4f;

    // Pulsing red text
    float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.0f) * 0.5f + 0.5f;
    Canvas->SetDrawColor(FColor(255, (uint8)(50 * Pulse), (uint8)(50 * Pulse), 255));
    Canvas->DrawText(GEngine->GetLargeFont(), TimerText, X, Y);

    // Draw "Press [F] to Broadcast (Sacrifice)" option
    FString SacrificeText = TEXT("[F] BROADCAST - Sacrifice to reveal entities");
    Canvas->SetDrawColor(FColor(200, 200, 200, 200));
    Canvas->DrawText(GEngine->GetSmallFont(), SacrificeText, X - 50.0f, Y + 40.0f);
}

void ASLHUD::DrawCascadeCountdown(float TimeRemaining)
{
    if (!Canvas) return;

    FString CountdownText = FString::Printf(TEXT("CASCADE: %.0f"), TimeRemaining);
    float X = Canvas->SizeX * 0.5f - 80.0f;
    float Y = 60.0f;

    // Flashing red
    bool bFlash = FMath::Fmod(GetWorld()->GetTimeSeconds(), 0.5f) < 0.25f;
    if (bFlash)
    {
        Canvas->SetDrawColor(FColor::Red);
        Canvas->DrawText(GEngine->GetLargeFont(), CountdownText, X, Y);
    }
}

void ASLHUD::DrawExtractionProgress(float Progress, int32 Phase)
{
    if (!Canvas) return;

    FString PhaseNames[] = { TEXT("SCANNING"), TEXT("DISCONNECTING"), TEXT("EXTRACTING") };
    FString PhaseText = Phase < 3 ? PhaseNames[Phase] : TEXT("UNKNOWN");

    float BarWidth = 300.0f;
    float BarHeight = 20.0f;
    float X = Canvas->SizeX * 0.5f - BarWidth * 0.5f;
    float Y = Canvas->SizeY * 0.7f;

    // Background
    Canvas->SetDrawColor(FColor(40, 40, 40, 200));
    Canvas->DrawRect(FLinearColor(0.15f, 0.15f, 0.15f, 0.8f), X, Y, BarWidth, BarHeight);

    // Progress fill
    Canvas->DrawRect(FLinearColor(0.2f, 0.8f, 0.3f, 0.9f), X, Y, BarWidth * Progress, BarHeight);

    // Phase text
    Canvas->SetDrawColor(FColor::White);
    Canvas->DrawText(GEngine->GetSmallFont(), PhaseText, X, Y - 20.0f);
}

void ASLHUD::DrawAirlockCompass(FVector AirlockDirection)
{
    if (!Canvas || !OwnerCharacter) return;

    // Simple compass at bottom of screen pointing toward airlock
    AirlockDirection.Z = 0.0f;
    AirlockDirection.Normalize();

    FVector PlayerForward = OwnerCharacter->GetActorForwardVector();
    PlayerForward.Z = 0.0f;
    PlayerForward.Normalize();

    float Angle = FMath::Atan2(
        FVector::CrossProduct(PlayerForward, AirlockDirection).Z,
        FVector::DotProduct(PlayerForward, AirlockDirection)
    );

    float CompassX = Canvas->SizeX * 0.5f + FMath::Sin(Angle) * 40.0f;
    float CompassY = Canvas->SizeY - 60.0f;

    // Draw simple arrow indicator
    Canvas->SetDrawColor(FColor(200, 100, 50, 200)); // Warm orange
    Canvas->DrawText(GEngine->GetSmallFont(), TEXT("^"), CompassX, CompassY);
    Canvas->DrawText(GEngine->GetSmallFont(), TEXT("AIRLOCK"),
        Canvas->SizeX * 0.5f - 25.0f, Canvas->SizeY - 40.0f);
}

void ASLHUD::DrawTeammateIndicators()
{
    // Draw minimal teammate markers (name + role icon)
    // Only visible when looking in their direction
}

void ASLHUD::DrawPerceptionDriftEffects(float DriftLevel)
{
    // These effects are invisible to the affected player conceptually,
    // but we render them for gameplay purposes
    // The KEY DESIGN: the player SEES the effects but doesn't know they're fake

    float NormalizedDrift = DriftLevel / 100.0f;

    if (NormalizedDrift > 0.3f)
    {
        // Subtle shadow movement in peripheral vision
        DriftShadowOffset = FMath::Sin(GetWorld()->GetTimeSeconds() * 0.5f) *
            NormalizedDrift * 50.0f;
    }

    if (NormalizedDrift > 0.5f)
    {
        // Color shifts in environment
        DriftColorShift = NormalizedDrift * 0.2f;
    }

    if (NormalizedDrift > 0.7f)
    {
        // Phantom entity silhouettes
        bDriftPhantomEntity = (FMath::FRand() < 0.01f * NormalizedDrift);
    }
}

void ASLHUD::DrawResyncProgress(float Progress)
{
    if (!Canvas) return;

    float BarWidth = 200.0f;
    float BarHeight = 15.0f;
    float X = Canvas->SizeX * 0.5f - BarWidth * 0.5f;
    float Y = Canvas->SizeY * 0.6f;

    Canvas->DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.8f), X, Y, BarWidth, BarHeight);
    Canvas->DrawRect(FLinearColor(0.3f, 0.6f, 1.0f, 0.9f), X, Y, BarWidth * Progress, BarHeight);

    Canvas->SetDrawColor(FColor::White);
    Canvas->DrawText(GEngine->GetSmallFont(), TEXT("RESYNCING..."), X, Y - 18.0f);
}

void ASLHUD::ToggleWristDevice()
{
    bWristDeviceOpen = !bWristDeviceOpen;
    // When open, player looks at wrist — camera tilts down, leaving player vulnerable
}

void ASLHUD::DrawWristDevice()
{
    if (!Canvas || !OwnerCharacter) return;

    // Wrist-mounted device shows:
    // - Equipment inventory with battery/charge status
    // - Current role info
    // - Carried cores and their values
    // - Team status (who's alive)

    float DeviceX = Canvas->SizeX * 0.3f;
    float DeviceY = Canvas->SizeY * 0.4f;
    float DeviceW = Canvas->SizeX * 0.4f;
    float DeviceH = Canvas->SizeY * 0.5f;

    // Background (dark screen with green text — old terminal aesthetic)
    Canvas->DrawRect(FLinearColor(0.02f, 0.05f, 0.02f, 0.95f),
        DeviceX, DeviceY, DeviceW, DeviceH);

    // Border
    Canvas->SetDrawColor(FColor(50, 200, 50, 200));

    float TextY = DeviceY + 15.0f;
    float LineHeight = 18.0f;

    // Role
    Canvas->DrawText(GEngine->GetSmallFont(),
        FString::Printf(TEXT("ROLE: %s"), *OwnerCharacter->GetRoleDisplayName().ToString()),
        DeviceX + 15.0f, TextY);
    TextY += LineHeight;

    // Health
    Canvas->DrawText(GEngine->GetSmallFont(),
        FString::Printf(TEXT("VITALS: %.0f%%"), OwnerCharacter->Health),
        DeviceX + 15.0f, TextY);
    TextY += LineHeight * 2;

    // Carried cores
    Canvas->DrawText(GEngine->GetSmallFont(),
        FString::Printf(TEXT("CORES: %d/%d"), OwnerCharacter->CarriedCores.Num(),
            OwnerCharacter->MaxCarryCores),
        DeviceX + 15.0f, TextY);
    TextY += LineHeight;

    for (const FSignalCore& Core : OwnerCharacter->CarriedCores)
    {
        Canvas->DrawText(GEngine->GetSmallFont(),
            FString::Printf(TEXT("  > %s: %d cr (%.0f%% integrity)"),
                *Core.CoreID.ToString(), Core.CreditValue, Core.Integrity),
            DeviceX + 15.0f, TextY);
        TextY += LineHeight;
    }
}

void ASLHUD::ApplyFrequencyBurnEffect(float Intensity, float Duration)
{
    FrequencyBurnIntensity = Intensity;
    FrequencyBurnTimer = Duration;
}

void ASLHUD::ApplyBroadcastDistortion(float Intensity)
{
    BroadcastDistortionIntensity = Intensity;
    // Drive post-process material parameters
}

void ASLHUD::ApplyCorruptionAmbientEffect(ECorruptionPhase Phase)
{
    // Adjust ambient post-process based on corruption phase
    // Quiet: normal
    // Stirring: slight desaturation
    // Active: green-tinted, scan lines
    // Critical: heavy distortion, color banding
    // Cascade: full visual breakdown
}

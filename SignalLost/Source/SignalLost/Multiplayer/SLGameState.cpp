#include "SLGameState.h"
#include "Net/UnrealNetwork.h"

ASLGameState::ASLGameState()
{
}

void ASLGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASLGameState, ActiveContract);
    DOREPLIFETIME(ASLGameState, bInMission);
    DOREPLIFETIME(ASLGameState, MissionTimer);
    DOREPLIFETIME(ASLGameState, TotalCoresExtracted);
    DOREPLIFETIME(ASLGameState, TotalCreditsEarned);
    DOREPLIFETIME(ASLGameState, CorruptionIndex);
    DOREPLIFETIME(ASLGameState, CurrentPhase);
    DOREPLIFETIME(ASLGameState, AlivePlayerCount);
    DOREPLIFETIME(ASLGameState, TotalPlayerCount);
    DOREPLIFETIME(ASLGameState, PlayersAtAirlock);
    DOREPLIFETIME(ASLGameState, ActiveEntityCount);
    DOREPLIFETIME(ASLGameState, bBroadcastSpawned);
    DOREPLIFETIME(ASLGameState, bCascadeActive);
    DOREPLIFETIME(ASLGameState, CascadeCountdown);
    DOREPLIFETIME(ASLGameState, WeeklyContractSeed);
    DOREPLIFETIME(ASLGameState, WeeklyContractName);
}

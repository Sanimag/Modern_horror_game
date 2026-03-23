#include "SignalLost.h"

DEFINE_LOG_CATEGORY(LogSignalLost);
IMPLEMENT_PRIMARY_GAME_MODULE(FSignalLostModule, SignalLost, "SignalLost");

void FSignalLostModule::StartupModule()
{
    UE_LOG(LogSignalLost, Log, TEXT("Signal Lost module initialized"));
}

void FSignalLostModule::ShutdownModule()
{
    UE_LOG(LogSignalLost, Log, TEXT("Signal Lost module shutdown"));
}

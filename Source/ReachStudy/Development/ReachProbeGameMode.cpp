#include "Development/ReachProbeGameMode.h"
#include "Hand/ReachHandPawn.h"

AReachProbeGameMode::AReachProbeGameMode()
{
    DefaultPawnClass = AReachHandPawn::StaticClass();
}

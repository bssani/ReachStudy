#include "ReachGameMode.h"
#include "Hand/ReachHandPawn.h"

AReachGameMode::AReachGameMode()
{
    DefaultPawnClass = AReachHandPawn::StaticClass();
}

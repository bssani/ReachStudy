#include "ReachGameMode.h"
#include "Hand/ReachHandPawn.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogReachCalibration, Log, All);

AReachGameMode::AReachGameMode()
{
    DefaultPawnClass = AReachHandPawn::StaticClass();
}

void AReachGameMode::StartPlay()
{
    // Alignment components require Stage and cm before BeginPlay (S1 D3).
    GetWorld()->GetWorldSettings()->WorldToMeters = 100;
    UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Stage);
    UE_LOG(LogReachCalibration, Log, TEXT("Tracking origin Stage, WorldToMeters 100 (S1)"));
    Super::StartPlay();
}

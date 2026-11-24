#include "Screen/ReachScreenMessageSubsystem.h"

#include "ReachCalibrationGameMode.h"
#include "CoreGlobals.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogReachScreenMessages, Log, All);

bool UReachScreenMessageSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE)
        && Super::ShouldCreateSubsystem(Outer);
}

void UReachScreenMessageSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (InWorld.GetAuthGameMode<AReachCalibrationGameMode>())
    {
        UE_LOG(LogReachScreenMessages, Log, TEXT("Screen messages kept (calibration level, S1)"));
        return;
    }
    bPreviousMessagesEnabled = GAreScreenMessagesEnabled;
    bMessagesDisabled = true;
    GAreScreenMessagesEnabled = false;
    UE_LOG(LogReachScreenMessages, Log, TEXT("Screen messages disabled (S4b)"));
}

void UReachScreenMessageSubsystem::Deinitialize()
{
    // Restore only worlds that changed the global setting (S4b D1).
    if (bMessagesDisabled)
    {
        GAreScreenMessagesEnabled = bPreviousMessagesEnabled;
        bMessagesDisabled = false;
    }
    Super::Deinitialize();
}

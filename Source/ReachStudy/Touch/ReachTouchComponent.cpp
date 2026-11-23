#include "Touch/ReachTouchComponent.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "Touch/ReachTouchSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogReachTouchComponent, Log, All);

EReachTouchPhase UReachTouchComponent::GetPhase() const
{
    switch (State.Phase)
    {
    case EReachContactPhase::Dwelling: return EReachTouchPhase::Dwelling;
    case EReachContactPhase::Touching: return EReachTouchPhase::Touching;
    default: return EReachTouchPhase::Idle;
    }
}

void UReachTouchComponent::BeginPlay()
{
    Super::BeginPlay();
    UMeshComponent* Parent = Cast<UMeshComponent>(GetAttachParent());
    if (!Parent)
    {
        UE_LOG(LogReachTouchComponent, Warning, TEXT("ReachTouchComponent parent is not a mesh: %s"), *GetPathName());
        return;
    }
    if (UReachTouchSubsystem* System = GetWorld()->GetSubsystem<UReachTouchSubsystem>())
    {
        System->RegisterTarget(this, Parent);
    }
}

void UReachTouchComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UReachTouchSubsystem* System = GetWorld()->GetSubsystem<UReachTouchSubsystem>())
    {
        System->UnregisterTarget(this);
    }
    Super::EndPlay(EndPlayReason);
}

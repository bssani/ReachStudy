#include "Hand/ReachHandPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Interaction/IsdkGrabberComponent.h"
#include "MotionControllerComponent.h"
#include "Rig/IsdkGrabInteractionRigComponent.h"
#include "Rig/IsdkPokeInteractionRigComponent.h"
#include "Rig/IsdkPokeLimiterRigModifier.h"
#include "Rig/IsdkRayInteractionRigComponent.h"

namespace
{
void ConfigurePokeOnly(UIsdkHandRigComponent* Rig)
{
    // Preserve SDK internal references; permit near poke only (S4 D7).
    for (UActorComponent* Interaction : {static_cast<UActorComponent*>(Rig->GetRayInteraction()),
        static_cast<UActorComponent*>(Rig->GetGrabInteraction())})
    {
        Interaction->bAutoActivate = false;
        Interaction->Deactivate();
    }
    Rig->GetRayInteraction()->RayVisualClass = nullptr;
    Rig->GetGrabber()->SetGrabDetectionTypeAllowed(EIsdkGrabDetectorType::HandGrab, false);
    Rig->GetGrabber()->SetGrabDetectionTypeAllowed(EIsdkGrabDetectorType::DistanceGrab, false);
    Rig->GetGrabber()->SetGrabDetectionTypeAllowed(EIsdkGrabDetectorType::RayGrab, false);
}
}

UReachHandRigLeft::UReachHandRigLeft()
{
    RigModifiersToSpawn.Add(UIsdkPokeLimiterRigModifier::StaticClass());
    bAutoBindInputActions = false;
    ConfigurePokeOnly(this);
}

UReachHandRigRight::UReachHandRigRight()
{
    RigModifiersToSpawn.Add(UIsdkPokeLimiterRigModifier::StaticClass());
    bAutoBindInputActions = false;
    ConfigurePokeOnly(this);
}

AReachHandPawn::AReachHandPawn()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin")));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(RootComponent);
    Camera->bLockToHmd = true;
    LeftMotion = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftHandTracking"));
    RightMotion = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightHandTracking"));
    LeftMotion->SetupAttachment(RootComponent);
    RightMotion->SetupAttachment(RootComponent);
    LeftMotion->SetTrackingMotionSource(TEXT("Left"));
    RightMotion->SetTrackingMotionSource(TEXT("Right"));
    LeftMotion->bDisableLowLatencyUpdate = true;
    RightMotion->bDisableLowLatencyUpdate = true;
    LeftRig = CreateDefaultSubobject<UReachHandRigLeft>(TEXT("LeftHandRig"));
    RightRig = CreateDefaultSubobject<UReachHandRigRight>(TEXT("RightHandRig"));
    LeftRig->SetupAttachment(LeftMotion);
    RightRig->SetupAttachment(RightMotion);
}

FReachHandSample AReachHandPawn::ReadHand(EControllerHand Hand, EReachContactPart Part) const
{
    FXRHandTrackingState Tracking;
    UHeadMountedDisplayFunctionLibrary::GetHandTrackingState(GetWorld(), EXRSpaceType::UnrealWorldSpace, Hand, Tracking);
    return ReachContact::ReadSample(Tracking, Part);
}

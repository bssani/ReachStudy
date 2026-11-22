#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Rig/IsdkHandRigComponent.h"
#include "Contact/ReachContactQuery.h"
#include "ReachHandPawn.generated.h"

class UCameraComponent;
class UMotionControllerComponent;

UCLASS()
class REACHSTUDY_API UReachHandRigLeft : public UIsdkHandRigComponentLeft
{
    GENERATED_BODY()
public:
    UReachHandRigLeft();
};

UCLASS()
class REACHSTUDY_API UReachHandRigRight : public UIsdkHandRigComponentRight
{
    GENERATED_BODY()
public:
    UReachHandRigRight();
};

UCLASS()
class REACHSTUDY_API AReachHandPawn : public APawn
{
    GENERATED_BODY()
public:
    AReachHandPawn();
    FReachHandSample ReadHand(EControllerHand Hand, EReachContactPart Part) const;

private:
    UPROPERTY(VisibleAnywhere, Category = "Reach")
    TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, Category = "Reach")
    TObjectPtr<UMotionControllerComponent> LeftMotion;
    UPROPERTY(VisibleAnywhere, Category = "Reach")
    TObjectPtr<UMotionControllerComponent> RightMotion;
    UPROPERTY(VisibleAnywhere, Category = "Reach")
    TObjectPtr<UReachHandRigLeft> LeftRig;
    UPROPERTY(VisibleAnywhere, Category = "Reach")
    TObjectPtr<UReachHandRigRight> RightRig;
};

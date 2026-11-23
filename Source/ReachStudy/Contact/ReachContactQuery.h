#pragma once

#include "CoreMinimal.h"
#include "HeadMountedDisplayTypes.h"
#include "InputCoreTypes.h"
#include "Contact/ReachContact.h"
#include "ReachContactQuery.generated.h"

class AActor;
class UMaterialInterface;
class UMeshComponent;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EReachContactPart : uint8
{
    IndexTip,
    Fingertips,
    WholeHand
};

UENUM(BlueprintType)
enum class EReachContactMethod : uint8
{
    SimpleDistance,
    ComplexOverlap
};

struct FReachJointSample
{
    EHandKeypoint Keypoint = EHandKeypoint::Palm;
    FVector WorldPosition = FVector::ZeroVector;
    float RadiusMm = 0.0f;
};

struct FReachHandSample
{
    bool bTracked = false;
    TArray<FReachJointSample> Joints;
};

struct FReachContactQueryResult
{
    float DistanceMm = -1.0f;
    float JointRadiusMm = 0.0f;
    EHandKeypoint NearestJoint = EHandKeypoint::Palm;
    bool bHasCollision = false;
    EReachContactCondition Simple = EReachContactCondition::None;
    EReachContactCondition Complex = EReachContactCondition::None;
};

namespace ReachContact
{
    TArray<EHandKeypoint> Keypoints(EReachContactPart Part);
    FReachHandSample ReadSample(const FXRHandTrackingState& Tracking, EReachContactPart Part);
    FReachHandSample ReadHand(const UObject* WorldContext, EControllerHand Hand, EReachContactPart Part);
    FReachContactQueryResult QueryComponent(UPrimitiveComponent* Target, const FReachHandSample& Hand,
        float ContactDistanceMm, float ReleaseMarginMm, bool bUseJointRadius);
    bool IsWithinContactBounds(UPrimitiveComponent* Target, const FReachHandSample& Hand,
        float ContactDistanceMm, float ReleaseMarginMm);
    FReachContactQueryResult Query(AActor* Target, const FReachHandSample& Hand,
        float ContactDistanceMm, float ReleaseMarginMm, bool bUseJointRadius);
    void ApplyOverlay(AActor* Target, UMaterialInterface* Material);
    void ApplyOverlay(UMeshComponent* Target, UMaterialInterface* Material);
}

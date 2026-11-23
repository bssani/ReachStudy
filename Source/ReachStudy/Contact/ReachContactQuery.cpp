#include "Contact/ReachContactQuery.h"

#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "PhysicsEngine/BodyInstance.h"

TArray<EHandKeypoint> ReachContact::Keypoints(EReachContactPart Part)
{
    if (Part == EReachContactPart::IndexTip) { return {EHandKeypoint::IndexTip}; }
    if (Part == EReachContactPart::Fingertips)
    {
        return {EHandKeypoint::ThumbTip, EHandKeypoint::IndexTip, EHandKeypoint::MiddleTip,
            EHandKeypoint::RingTip, EHandKeypoint::LittleTip};
    }
    TArray<EHandKeypoint> Result;
    for (int32 Index = 0; Index < EHandKeypointCount; ++Index)
    {
        Result.Add(static_cast<EHandKeypoint>(Index));
    }
    return Result;
}

FReachHandSample ReachContact::ReadSample(const FXRHandTrackingState& Tracking, EReachContactPart Part)
{
    FReachHandSample Result;
    if (!Tracking.bValid || Tracking.TrackingStatus != ETrackingStatus::Tracked) { return Result; }
    for (EHandKeypoint Keypoint : Keypoints(Part))
    {
        const int32 Index = static_cast<int32>(Keypoint);
        if (!Tracking.HandKeyLocations.IsValidIndex(Index)
            || Tracking.HandKeyLocations[Index].ContainsNaN())
        {
            // Reject the entire hand instead of partial data (S2 D2).
            return {};
        }
        float RadiusMm = 0.0f;
        if (Tracking.HandKeyRadii.IsValidIndex(Index) && FMath::IsFinite(Tracking.HandKeyRadii[Index]))
        {
            // UE is cm, settings and readout are mm (S2 D5).
            RadiusMm = FMath::Max(0.0f, Tracking.HandKeyRadii[Index] * 10.0f);
        }
        Result.Joints.Add({Keypoint, Tracking.HandKeyLocations[Index], RadiusMm});
    }
    Result.bTracked = true;
    return Result;
}

FReachHandSample ReachContact::ReadHand(const UObject* WorldContext, EControllerHand Hand, EReachContactPart Part)
{
    FXRHandTrackingState Tracking;
    UHeadMountedDisplayFunctionLibrary::GetHandTrackingState(WorldContext ? WorldContext->GetWorld() : nullptr,
        EXRSpaceType::UnrealWorldSpace, Hand, Tracking);
    return ReadSample(Tracking, Part);
}

FReachContactQueryResult ReachContact::QueryComponent(UPrimitiveComponent* Target, const FReachHandSample& Hand,
    float ContactDistanceMm, float ReleaseMarginMm, bool bUseJointRadius)
{
    FReachContactQueryResult Result;
    if (!IsValid(Target)) { return Result; }
    FBodyInstance* Body = Target->GetBodyInstance();
    if (!Target->IsQueryCollisionEnabled() || !Body || !Body->IsValidBodyInstance()) { return Result; }
    Result.bHasCollision = true;
    if (Hand.bTracked)
    {
        for (const FReachJointSample& Joint : Hand.Joints)
        {
            const float RadiusMm = bUseJointRadius ? Joint.RadiusMm : 0.0f;
            float SquaredDistanceCm = 0.0f;
            FVector ClosestPoint;
            if (Target->GetSquaredDistanceToCollision(Joint.WorldPosition, SquaredDistanceCm, ClosestPoint))
            {
                // UE is cm, settings and readout are mm (S2 D5).
                const float DistanceMm = FMath::Max(0.0f, FMath::Sqrt(SquaredDistanceCm) * 10.0f - RadiusMm);
                if (Result.DistanceMm < 0.0f || DistanceMm < Result.DistanceMm)
                {
                    Result.DistanceMm = DistanceMm;
                    Result.JointRadiusMm = Joint.RadiusMm;
                    Result.NearestJoint = Joint.Keypoint;
                }
            }
            // Settings are mm, Chaos sphere radius is cm (S2 D5).
            const float EnterRadiusCm = (ContactDistanceMm + RadiusMm) * 0.1f;
            const float HoldRadiusCm = (ContactDistanceMm + ReleaseMarginMm + RadiusMm) * 0.1f;
            const bool bEnter = Body->OverlapTest(Joint.WorldPosition, FQuat::Identity,
                FCollisionShape::MakeSphere(EnterRadiusCm), nullptr, true);
            const bool bHold = bEnter || Body->OverlapTest(Joint.WorldPosition, FQuat::Identity,
                FCollisionShape::MakeSphere(HoldRadiusCm), nullptr, true);
            if (bEnter) { Result.Complex = EReachContactCondition::Enter; }
            else if (bHold && Result.Complex == EReachContactCondition::None)
            {
                Result.Complex = EReachContactCondition::Hold;
            }
        }
    }
    if (Result.DistanceMm >= 0.0f)
    {
        Result.Simple = Result.DistanceMm <= ContactDistanceMm ? EReachContactCondition::Enter
            : Result.DistanceMm <= ContactDistanceMm + ReleaseMarginMm ? EReachContactCondition::Hold
            : EReachContactCondition::None;
    }
    return Result;
}

FReachContactQueryResult ReachContact::Query(AActor* Target, const FReachHandSample& Hand,
    float ContactDistanceMm, float ReleaseMarginMm, bool bUseJointRadius)
{
    FReachContactQueryResult Result;
    if (!IsValid(Target)) { return Result; }
    TInlineComponentArray<UStaticMeshComponent*> Meshes(Target);
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        const FReachContactQueryResult One = QueryComponent(Mesh, Hand, ContactDistanceMm, ReleaseMarginMm, bUseJointRadius);
        Result.bHasCollision |= One.bHasCollision;
        if (One.DistanceMm >= 0.0f && (Result.DistanceMm < 0.0f || One.DistanceMm < Result.DistanceMm))
        {
            Result.DistanceMm = One.DistanceMm;
            Result.JointRadiusMm = One.JointRadiusMm;
            Result.NearestJoint = One.NearestJoint;
        }
        if (One.Simple > Result.Simple) { Result.Simple = One.Simple; }
        if (One.Complex > Result.Complex) { Result.Complex = One.Complex; }
    }
    return Result;
}

bool ReachContact::IsWithinContactBounds(UPrimitiveComponent* Target, const FReachHandSample& Hand,
    float ContactDistanceMm, float ReleaseMarginMm)
{
    if (!IsValid(Target) || !Hand.bTracked) { return false; }
    FBox Bounds = Target->Bounds.GetBox();
    if (FBodyInstance* Body = Target->GetBodyInstance(); Body && Body->IsValidBodyInstance())
    {
        const FBox BodyBounds = Body->GetBodyBounds();
        if (BodyBounds.IsValid) { Bounds += BodyBounds; }
    }
    if (!Bounds.IsValid) { return true; }
    float MaxRadiusMm = 0.0f;
    for (const FReachJointSample& Joint : Hand.Joints)
    {
        MaxRadiusMm = FMath::Max(MaxRadiusMm, Joint.RadiusMm);
    }
    // UE is cm; include simple bodies outside rendering bounds (S3 D7).
    // Keep inclusive boundaries in FVector's double precision (S3 E12).
    const double PaddingCm = (static_cast<double>(ContactDistanceMm) + ReleaseMarginMm + MaxRadiusMm) * 0.1;
    Bounds = Bounds.ExpandBy(PaddingCm);
    for (const FReachJointSample& Joint : Hand.Joints)
    {
        if (Bounds.IsInsideOrOn(Joint.WorldPosition)) { return true; }
    }
    return false;
}

void ReachContact::ApplyOverlay(UMeshComponent* Target, UMaterialInterface* Material)
{
    if (IsValid(Target)) { Target->SetOverlayMaterial(Material); }
}

void ReachContact::ApplyOverlay(AActor* Target, UMaterialInterface* Material)
{
    if (!IsValid(Target)) { return; }
    TInlineComponentArray<UStaticMeshComponent*> Meshes(Target);
    for (UStaticMeshComponent* Mesh : Meshes) { Mesh->SetOverlayMaterial(Material); }
}

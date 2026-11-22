#include "Contact/ReachContactQuery.h"

#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
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

FReachContactQueryResult ReachContact::Query(AActor* Target, const FReachHandSample& Hand,
    float ContactDistanceMm, float ReleaseMarginMm, bool bUseJointRadius)
{
    FReachContactQueryResult Result;
    if (!IsValid(Target)) { return Result; }
    TInlineComponentArray<UStaticMeshComponent*> Meshes(Target);
    for (UStaticMeshComponent* Mesh : Meshes)
    {
        FBodyInstance* Body = Mesh->GetBodyInstance();
        if (!Mesh->IsQueryCollisionEnabled() || !Body || !Body->IsValidBodyInstance()) { continue; }
        Result.bHasCollision = true;
        if (!Hand.bTracked) { continue; }
        for (const FReachJointSample& Joint : Hand.Joints)
        {
            const float RadiusMm = bUseJointRadius ? Joint.RadiusMm : 0.0f;
            float SquaredDistanceCm = 0.0f;
            FVector ClosestPoint;
            if (Mesh->GetSquaredDistanceToCollision(Joint.WorldPosition, SquaredDistanceCm, ClosestPoint))
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

void ReachContact::ApplyOverlay(AActor* Target, UMaterialInterface* Material)
{
    if (!IsValid(Target)) { return; }
    TInlineComponentArray<UStaticMeshComponent*> Meshes(Target);
    for (UStaticMeshComponent* Mesh : Meshes) { Mesh->SetOverlayMaterial(Material); }
}

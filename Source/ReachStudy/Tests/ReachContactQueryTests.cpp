#include "Misc/AutomationTest.h"
#include "Contact/ReachContactQuery.h"
#include "Development/ReachContactProbe.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/UObjectGlobals.h"
#include <limits>

namespace
{
using C = EReachContactCondition;
using P = EReachContactPhase;

struct FProbeWorld
{
    UWorld* World = nullptr;
    AActor* Target = nullptr;
    UStaticMeshComponent* Mesh = nullptr;

    explicit FProbeWorld(bool bComplexOnly = false)
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(true).ShouldSimulatePhysics(false)
            .EnableTraceCollision(true).CreateNavigation(false).CreateAISystem(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
            ERHIFeatureLevel::Num, &Values);
        UStaticMesh* SourceMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        UStaticMesh* TestMesh = DuplicateObject<UStaticMesh>(SourceMesh, GetTransientPackage());
        UBodySetup* Body = TestMesh->GetBodySetup();
        Body->CollisionTraceFlag = bComplexOnly ? CTF_UseComplexAsSimple : CTF_UseSimpleAndComplex;
        if (bComplexOnly) { Body->AggGeom.EmptyElements(); }
        Body->InvalidatePhysicsData();
        Body->CreatePhysicsMeshes();
        Target = World->SpawnActor<AActor>();
        Mesh = NewObject<UStaticMeshComponent>(Target);
        Target->AddInstanceComponent(Mesh);
        Target->SetRootComponent(Mesh);
        Mesh->SetStaticMesh(TestMesh);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        Mesh->RegisterComponent();
    }

    ~FProbeWorld() { World->DestroyWorld(false); }
};

FReachHandSample Point(float X, float RadiusMm = 0.0f)
{
    FReachHandSample Hand;
    Hand.bTracked = true;
    Hand.Joints.Add({EHandKeypoint::IndexTip, FVector(X, 0.0, 0.0), RadiusMm});
    return Hand;
}

FXRHandTrackingState FullTracking()
{
    FXRHandTrackingState Tracking;
    Tracking.bValid = true;
    Tracking.TrackingStatus = ETrackingStatus::Tracked;
    Tracking.HandKeyLocations.Init(FVector(100.0, 0.0, 0.0), EHandKeypointCount);
    Tracking.HandKeyRadii.Init(0.8f, EHandKeypointCount);
    return Tracking;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachJointModesTest, "ReachStudy.S2.Query.E9_JointsAndValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachJointModesTest::RunTest(const FString& Parameters)
{
    const TArray<EHandKeypoint> Index = ReachContact::Keypoints(EReachContactPart::IndexTip);
    TestEqual(TEXT("E9: index count"), Index.Num(), 1);
    TestTrue(TEXT("E9: index name"), Index[0] == EHandKeypoint::IndexTip);
    const TArray<EHandKeypoint> Tips = ReachContact::Keypoints(EReachContactPart::Fingertips);
    TestEqual(TEXT("E9: five tips"), Tips.Num(), 5);
    TestTrue(TEXT("E9: tip identities"), Tips == TArray<EHandKeypoint>{EHandKeypoint::ThumbTip,
        EHandKeypoint::IndexTip, EHandKeypoint::MiddleTip, EHandKeypoint::RingTip, EHandKeypoint::LittleTip});
    const TArray<EHandKeypoint> Whole = ReachContact::Keypoints(EReachContactPart::WholeHand);
    TestEqual(TEXT("E9: whole hand count"), Whole.Num(), 26);
    for (int32 Joint = 0; Joint < Whole.Num(); ++Joint)
    {
        TestEqual(TEXT("E9: all keypoint identities"), static_cast<int32>(Whole[Joint]), Joint);
    }
    FXRHandTrackingState Tracking = FullTracking();
    FReachHandSample Sample = ReachContact::ReadSample(Tracking, EReachContactPart::Fingertips);
    TestTrue(TEXT("Valid raw hand accepted"), Sample.bTracked && Sample.Joints.Num() == 5);
    TestEqual(TEXT("Radius converted from cm to mm"), Sample.Joints[0].RadiusMm, 8.0f);
    Tracking.HandKeyLocations[static_cast<int32>(EHandKeypoint::ThumbTip)].X = std::numeric_limits<double>::quiet_NaN();
    Sample = ReachContact::ReadSample(Tracking, EReachContactPart::Fingertips);
    TestFalse(TEXT("One invalid selected joint rejects entire hand"), Sample.bTracked);
    TestEqual(TEXT("No partial joint subset"), Sample.Joints.Num(), 0);
    TestTrue(TEXT("Invalid unselected joint does not invalidate index mode"), ReachContact::ReadSample(Tracking, EReachContactPart::IndexTip).bTracked);
    Tracking = FullTracking();
    Tracking.HandKeyLocations.SetNum(10);
    TestFalse(TEXT("Missing selected location rejects hand"), ReachContact::ReadSample(Tracking, EReachContactPart::Fingertips).bTracked);
    Tracking = FullTracking();
    Tracking.bValid = false;
    TestFalse(TEXT("Invalid tracking rejected"), ReachContact::ReadSample(Tracking, EReachContactPart::IndexTip).bTracked);
    Tracking.bValid = true;
    Tracking.TrackingStatus = ETrackingStatus::InertialOnly;
    TestFalse(TEXT("Untracked confidence rejected"), ReachContact::ReadSample(Tracking, EReachContactPart::IndexTip).bTracked);
    Tracking = FullTracking();
    Tracking.HandKeyRadii.Empty();
    TestEqual(TEXT("Missing radius defaults to zero"), ReachContact::ReadSample(Tracking, EReachContactPart::IndexTip).Joints[0].RadiusMm, 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachSimpleDistanceTest, "ReachStudy.S2.Query.E4_E6_E11_E15b_SimpleDistance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachSimpleDistanceTest::RunTest(const FString& Parameters)
{
    FProbeWorld Fixture;
    FReachContactQueryResult Result = ReachContact::Query(Fixture.Target, Point(50.5f), 10.0f, 5.0f, false);
    TestTrue(TEXT("E11: box outside is 5 mm within 0.1 mm"), FMath::IsNearlyEqual(Result.DistanceMm, 5.0f, 0.1f));
    TestTrue(TEXT("E11: outside enters"), Result.Simple == C::Enter);
    Result = ReachContact::Query(Fixture.Target, Point(0.0f), 10.0f, 5.0f, false);
    TestEqual(TEXT("E11: convex inside is zero"), Result.DistanceMm, 0.0f);
    Result = ReachContact::Query(Fixture.Target, Point(51.2f), 10.0f, 5.0f, false);
    TestTrue(TEXT("E4: 12 mm gives hold"), Result.Simple == C::Hold);
    Result = ReachContact::Query(Fixture.Target, Point(51.6f), 10.0f, 5.0f, false);
    TestTrue(TEXT("E5: 16 mm gives none"), Result.Simple == C::None);
    FReachContactState State;
    State.Step({true, C::Enter}, {}, 0.0, 0.0, 0.1);
    State.Step({true, C::Enter}, {}, 0.01, 0.0, 0.1);
    for (int32 Frame = 0; Frame < 20; ++Frame)
    {
        Result = ReachContact::Query(Fixture.Target, Point(Frame % 2 ? 50.9f : 51.1f), 10.0f, 5.0f, false);
        TestFalse(TEXT("E6: 9/11 mm keeps touching"), State.Step({true, Result.Simple}, {}, 0.01, 0.0, 0.1));
    }
    Result = ReachContact::Query(Fixture.Target, Point(51.5f, 8.0f), 10.0f, 5.0f, true);
    TestTrue(TEXT("E15b: 15 mm minus 8 mm is 7 mm"), FMath::IsNearlyEqual(Result.DistanceMm, 7.0f, 0.1f));
    TestTrue(TEXT("E15b: corrected distance enters"), Result.Simple == C::Enter);
    Result = ReachContact::Query(Fixture.Target, Point(51.5f, 8.0f), 10.0f, 5.0f, false);
    TestTrue(TEXT("Radius off measures joint center"), FMath::IsNearlyEqual(Result.DistanceMm, 15.0f, 0.1f));
    TestEqual(TEXT("Readout includes radius while off"), Result.JointRadiusMm, 8.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachComplexOverlapTest, "ReachStudy.S2.Query.E12_E15_ComplexOverlap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachComplexOverlapTest::RunTest(const FString& Parameters)
{
    FProbeWorld Fixture(true);
    FReachContactQueryResult Result = ReachContact::Query(Fixture.Target, Point(50.5f), 10.0f, 5.0f, false);
    TestTrue(TEXT("E12: complex-only has no simple distance"), Result.DistanceMm < 0.0f && Result.Simple == C::None);
    TestTrue(TEXT("E13: complex surface within 5 mm enters"), Result.Complex == C::Enter);
    Result = ReachContact::Query(Fixture.Target, Point(51.2f), 10.0f, 5.0f, false);
    TestTrue(TEXT("E13b: 12 mm holds without enter"), Result.Complex == C::Hold);
    Result = ReachContact::Query(Fixture.Target, Point(20.0f), 10.0f, 5.0f, false);
    TestTrue(TEXT("E14: 30 cm from inner surface has no contact"), Result.Complex == C::None);
    Result = ReachContact::Query(Fixture.Target, Point(47.0f), 10.0f, 5.0f, false);
    TestTrue(TEXT("E14: 3 cm behind surface has no contact"), Result.Complex == C::None);
    Result = ReachContact::Query(Fixture.Target, Point(51.5f, 8.0f), 10.0f, 5.0f, true);
    TestTrue(TEXT("Joint radius extends overlap sphere to hand surface"), Result.Complex == C::Enter);
    Fixture.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Result = ReachContact::Query(Fixture.Target, Point(50.5f), 10.0f, 5.0f, false);
    TestTrue(TEXT("E15: disabled collision has n/a and no collision"), Result.DistanceMm < 0.0f && !Result.bHasCollision);
    TestTrue(TEXT("E15: neither method reports contact"), Result.Simple == C::None && Result.Complex == C::None);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachPalmModeTest, "ReachStudy.S2.Query.E10_PalmMode", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachPalmModeTest::RunTest(const FString& Parameters)
{
    FProbeWorld Fixture;
    FXRHandTrackingState Tracking = FullTracking();
    Tracking.HandKeyLocations[static_cast<int32>(EHandKeypoint::Palm)] = FVector(50.5, 0.0, 0.0);
    FReachContactQueryResult Result = ReachContact::Query(Fixture.Target,
        ReachContact::ReadSample(Tracking, EReachContactPart::Fingertips), 10.0f, 5.0f, false);
    TestTrue(TEXT("E10: palm ignored by fingertips in A and B"), Result.Simple == C::None && Result.Complex == C::None);
    Result = ReachContact::Query(Fixture.Target,
        ReachContact::ReadSample(Tracking, EReachContactPart::WholeHand), 10.0f, 5.0f, false);
    TestTrue(TEXT("E10: palm enters in whole hand in A and B"), Result.Simple == C::Enter && Result.Complex == C::Enter);
    TestTrue(TEXT("Nearest joint readout identifies palm"), Result.NearestJoint == EHandKeypoint::Palm);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachProbeKeysTest, "ReachStudy.S2.Probe.E16_Keys", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachProbeKeysTest::RunTest(const FString& Parameters)
{
    FProbeWorld Fixture;
    AReachContactProbe* Probe = Fixture.World->SpawnActor<AReachContactProbe>();
    for (int32 Press = 0; Press < 100; ++Press)
    {
        for (const FKey& Key : {EKeys::Equals, EKeys::Period, EKeys::RightBracket, EKeys::Zero}) { Probe->HandleKey(Key); }
    }
    TestEqual(TEXT("Contact maximum 50 mm"), Probe->ContactDistanceMm, 50.0f);
    TestEqual(TEXT("Release maximum 20 mm"), Probe->ReleaseMarginMm, 20.0f);
    TestEqual(TEXT("Dwell maximum 2 s"), Probe->DwellSeconds, 2.0f);
    TestEqual(TEXT("Grace maximum 1 s"), Probe->TrackingGraceSeconds, 1.0f);
    for (int32 Press = 0; Press < 100; ++Press)
    {
        for (const FKey& Key : {EKeys::Hyphen, EKeys::Comma, EKeys::LeftBracket, EKeys::Nine}) { Probe->HandleKey(Key); }
    }
    TestEqual(TEXT("Contact minimum 0 mm"), Probe->ContactDistanceMm, 0.0f);
    TestEqual(TEXT("Release minimum 0 mm"), Probe->ReleaseMarginMm, 0.0f);
    TestEqual(TEXT("Dwell minimum 0 s"), Probe->DwellSeconds, 0.0f);
    TestEqual(TEXT("Grace minimum 0 s"), Probe->TrackingGraceSeconds, 0.0f);
    Probe->HandleKey(EKeys::M);
    TestTrue(TEXT("M selects simple distance"), Probe->ContactMethod == EReachContactMethod::SimpleDistance);
    Probe->HandleKey(EKeys::M);
    TestTrue(TEXT("M selects complex overlap"), Probe->ContactMethod == EReachContactMethod::ComplexOverlap);
    Probe->HandleKey(EKeys::One);
    TestTrue(TEXT("1 selects index tip"), Probe->ContactPart == EReachContactPart::IndexTip);
    Probe->HandleKey(EKeys::Two);
    TestTrue(TEXT("2 selects five tips"), Probe->ContactPart == EReachContactPart::Fingertips);
    Probe->HandleKey(EKeys::Three);
    TestTrue(TEXT("3 selects whole hand"), Probe->ContactPart == EReachContactPart::WholeHand);
    Probe->HandleKey(EKeys::R);
    TestTrue(TEXT("R enables joint radius"), Probe->bUseJointRadius);
    Probe->HandleKey(EKeys::R);
    TestFalse(TEXT("R disables joint radius"), Probe->bUseJointRadius);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachOverlayTest, "ReachStudy.S2.Probe.E5_E17_OverlayRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachOverlayTest::RunTest(const FString& Parameters)
{
    FProbeWorld Fixture;
    const TArray<UMaterialInterface*> Before = Fixture.Mesh->GetMaterials();
    const FTransform TransformBefore = Fixture.Mesh->GetComponentTransform();
    UStaticMesh* MeshBefore = Fixture.Mesh->GetStaticMesh();
    const ECollisionEnabled::Type CollisionBefore = Fixture.Mesh->GetCollisionEnabled();
    AReachContactProbe* Probe = Fixture.World->SpawnActor<AReachContactProbe>();
    Probe->Targets.Add(Fixture.Target);
    Probe->DwellSeconds = 0.0f;
    Probe->ProcessFrame(Point(50.5f), {}, 0.01f);
    TestNull(TEXT("First tracked frame has no overlay"), Fixture.Mesh->GetOverlayMaterial());
    Probe->ProcessFrame(Point(50.5f), {}, 0.01f);
    TestNotNull(TEXT("Touching applies runtime overlay"), Fixture.Mesh->GetOverlayMaterial());
    TestTrue(TEXT("Probe exposes touching phase"), Probe->GetResults()[0].Phase == P::Touching);
    Probe->ProcessFrame({}, {}, 0.01f);
    TestNotNull(TEXT("Grace retains overlay"), Fixture.Mesh->GetOverlayMaterial());
    Probe->ProcessFrame(Point(51.6f), {}, 0.01f);
    TestNotNull(TEXT("Recovery frame retains overlay"), Fixture.Mesh->GetOverlayMaterial());
    Probe->ProcessFrame(Point(51.6f), {}, 0.01f);
    TestNull(TEXT("E5/E17: measured exit removes overlay"), Fixture.Mesh->GetOverlayMaterial());
    TestTrue(TEXT("Slot materials unchanged"), Fixture.Mesh->GetMaterials() == Before);
    TestTrue(TEXT("Transform unchanged"), Fixture.Mesh->GetComponentTransform().Equals(TransformBefore));
    TestTrue(TEXT("Mesh unchanged"), Fixture.Mesh->GetStaticMesh() == MeshBefore);
    TestTrue(TEXT("Collision unchanged"), Fixture.Mesh->GetCollisionEnabled() == CollisionBefore);
    return true;
}
#endif

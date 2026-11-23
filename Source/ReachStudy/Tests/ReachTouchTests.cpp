#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ReachTouchTestObserver.h"
#include "Touch/ReachTouchComponent.h"
#include "Touch/ReachTouchSettings.h"
#include "Touch/ReachTouchSubsystem.h"
#include "Door/ReachDoorVariants.h"
#include "ReachGameMode.h"
#include "Hand/ReachHandPawn.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"

namespace
{
struct FTouchWorld
{
    UWorld* World;
    UReachTouchSubsystem* System;
    AReachTouchSettings* Settings = nullptr;

    explicit FTouchWorld(bool bSettings = true, EWorldType::Type Type = EWorldType::Game)
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(true).ShouldSimulatePhysics(false)
            .EnableTraceCollision(true).CreateNavigation(false).CreateAISystem(false);
        World = UWorld::CreateWorld(Type, false, NAME_None, nullptr, true,
            ERHIFeatureLevel::Num, &Values);
        GEngine->CreateNewWorldContext(Type).SetCurrentWorld(World);
        if (Type != EWorldType::Editor) { World->InitializeActorsForPlay(FURL()); }
        System = World->GetSubsystem<UReachTouchSubsystem>();
        if (bSettings)
        {
            Settings = World->SpawnActor<AReachTouchSettings>();
            Settings->ContactMethod = EReachContactMethod::SimpleDistance;
        }
    }

    AActor* Actor()
    {
        AActor* Owner = World->SpawnActor<AActor>();
        USceneComponent* Root = NewObject<USceneComponent>(Owner);
        Owner->AddInstanceComponent(Root);
        Owner->SetRootComponent(Root);
        Root->RegisterComponent();
        return Owner;
    }

    UStaticMeshComponent* Cube(AActor* Owner, FVector Position = FVector::ZeroVector)
    {
        UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
        Owner->AddInstanceComponent(Mesh);
        Mesh->SetupAttachment(Owner->GetRootComponent());
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        Mesh->SetRelativeLocation(Position);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        Mesh->RegisterComponent();
        return Mesh;
    }

    UReachTouchComponent* Touch(USceneComponent* Parent)
    {
        UReachTouchComponent* Component = NewObject<UReachTouchComponent>(Parent->GetOwner());
        Parent->GetOwner()->AddInstanceComponent(Component);
        Component->SetupAttachment(Parent);
        Component->RegisterComponent();
        return Component;
    }

    void Frame(const FXRHandTrackingState& Left, float Seconds = 0.011f,
        const FXRHandTrackingState& Right = FXRHandTrackingState())
    {
        System->ProcessFrame(Left, Right, Seconds);
    }

    ~FTouchWorld()
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->HasActorBegunPlay()) { It->RouteEndPlay(EEndPlayReason::Quit); }
        }
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    }
};

FXRHandTrackingState Hand(FVector Position = FVector(50.5, 0.0, 0.0))
{
    FXRHandTrackingState Result;
    Result.bValid = true;
    Result.TrackingStatus = ETrackingStatus::Tracked;
    Result.HandKeyLocations.Init(Position, EHandKeypointCount);
    Result.HandKeyRadii.Init(0.0f, EHandKeypointCount);
    return Result;
}

UReachTouchTestObserver* Observe(UReachTouchComponent* Component)
{
    UReachTouchTestObserver* Observer = NewObject<UReachTouchTestObserver>(Component->GetOwner());
    Component->OnTouchBegin.AddDynamic(Observer, &UReachTouchTestObserver::Begin);
    Component->OnTouchEnd.AddDynamic(Observer, &UReachTouchTestObserver::End);
    return Observer;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachTouchLifetimeTest, "ReachStudy.S3.E1_E2_E4_E6_E15_ContactLifetime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachTouchLifetimeTest::RunTest(const FString& Parameters)
{
    AddExpectedMessage(TEXT("Pawn is not ReachHandPawn"), EAutomationExpectedMessageFlags::Contains, 1);
    FTouchWorld F;
    AActor* Owner = F.Actor();
    UStaticMeshComponent* Mesh = F.Cube(Owner);
    UStaticMeshComponent* Neighbor = F.Cube(Owner, FVector(0.0, 200.0, 0.0));
    UMaterialInterface* Slot = Mesh->GetMaterial(0);
    const FTransform Transform = Mesh->GetComponentTransform();
    UReachTouchComponent* Touch = F.Touch(Mesh);
    UReachTouchTestObserver* Events = Observe(Touch);
    Owner->DispatchBeginPlay();
    F.Frame(Hand());
    TestTrue(TEXT("E1: first tracked frame skipped"), Touch->GetPhase() == EReachTouchPhase::Idle);
    F.Frame(Hand());
    TestTrue(TEXT("E1: dwelling"), Touch->GetPhase() == EReachTouchPhase::Dwelling);
    for (int32 Frame = 0; Frame < 12; ++Frame) { F.Frame(Hand()); }
    TestFalse(TEXT("E1: 0.143 s is still dwelling"), Touch->IsTouching());
    F.Frame(Hand());
    TestTrue(TEXT("E1: 0.154 s touches"), Touch->IsTouching());
    TestEqual(TEXT("E1: one begin"), Events->Begins, 1);
    TestNotNull(TEXT("E6: default overlay"), Mesh->GetOverlayMaterial());
    UMaterialInstanceDynamic* Default = Cast<UMaterialInstanceDynamic>(Mesh->GetOverlayMaterial());
    if (!TestNotNull(TEXT("E6: dynamic default material"), Default)) { return false; }
    FLinearColor Color;
    Default->GetVectorParameterValue(FMaterialParameterInfo(TEXT("ColorParam")), Color);
    TestEqual(TEXT("E6: settings color"), Color, F.Settings->TouchColor);
    F.Settings->TouchColor = FLinearColor::Red;
    F.Frame(Hand());
    Default->GetVectorParameterValue(FMaterialParameterInfo(TEXT("ColorParam")), Color);
    TestEqual(TEXT("E6: live color"), Color, FLinearColor::Red);
    TestNull(TEXT("E1: neighbor unchanged"), Neighbor->GetOverlayMaterial());
    TestEqual(TEXT("Slot unchanged"), Mesh->GetMaterial(0), Slot);
    TestTrue(TEXT("Transform unchanged"), Mesh->GetComponentTransform().Equals(Transform));
    TestTrue(TEXT("Collision unchanged"), Mesh->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
    F.Frame(Hand(FVector(51.2, 0.0, 0.0)));
    TestTrue(TEXT("E2: 12 mm holds"), Touch->IsTouching());
    F.Frame(Hand(FVector(51.6, 0.0, 0.0)));
    TestFalse(TEXT("E2: 16 mm releases"), Touch->IsTouching());
    TestNull(TEXT("E2: overlay cleared"), Mesh->GetOverlayMaterial());
    TestEqual(TEXT("E2: one end"), Events->Ends, 1);
    Touch->TouchMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    F.Settings->DwellSeconds = 0.0f;
    F.Frame(Hand());
    TestEqual(TEXT("E6: specified material"), Mesh->GetOverlayMaterial(), Touch->TouchMaterial.Get());
    F.Frame(FXRHandTrackingState(), 0.05f);
    TestTrue(TEXT("E4: short loss holds"), Touch->IsTouching());
    F.Frame(Hand(), 0.011f);
    TestTrue(TEXT("E4: recovery holds"), Touch->IsTouching());
    F.Frame(Hand());
    F.Frame(FXRHandTrackingState(), 0.2f);
    TestFalse(TEXT("E4: long loss clears"), Touch->IsTouching());
    F.Frame(Hand());
    TestFalse(TEXT("E4: first recovery frame skipped"), Touch->IsTouching());
    F.Frame(Hand());
    TestTrue(TEXT("Recovered contact"), Touch->IsTouching());
    const int32 Ends = Events->Ends;
    Owner->Destroy();
    TestNull(TEXT("E15: EndPlay clears overlay"), Mesh->GetOverlayMaterial());
    TestEqual(TEXT("E15: no EndPlay event"), Events->Ends, Ends);
    F.Frame(Hand());
    TestEqual(TEXT("E15: unregistered"), Events->Ends, Ends);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachTouchCountsTest, "ReachStudy.S3.E3_UnlimitedComponents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachTouchCountsTest::RunTest(const FString& Parameters)
{
    AddExpectedMessage(TEXT("Pawn is not ReachHandPawn"), EAutomationExpectedMessageFlags::Contains, 1);
    FTouchWorld F;
    F.Settings->DwellSeconds = 0.0f;
    TArray<UReachTouchComponent*> Targets;
    for (int32 Count : {3, 7})
    {
        AActor* Owner = F.Actor();
        for (int32 Index = 0; Index < Count; ++Index)
        {
            Targets.Add(F.Touch(F.Cube(Owner, FVector(0.0, Targets.Num() * 200.0, 0.0))));
        }
        Owner->DispatchBeginPlay();
    }
    F.Frame(Hand()); F.Frame(Hand());
    TestTrue(TEXT("E3: first target touches"), Targets[0]->IsTouching());
    for (int32 Index = 1; Index < Targets.Num(); ++Index)
    {
        TestFalse(TEXT("E3: other targets idle"), Targets[Index]->IsTouching());
    }
    for (int32 Index = 0; Index < Targets.Num(); ++Index)
    {
        F.Frame(Hand(FVector(50.5, Index * 200.0, 0.0)));
        TestTrue(TEXT("E3: every registered target can touch"), Targets[Index]->IsTouching());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachTouchModesTest, "ReachStudy.S3.E5_E7_PartsAndMethods", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachTouchModesTest::RunTest(const FString& Parameters)
{
    AddExpectedMessage(TEXT("Pawn is not ReachHandPawn"), EAutomationExpectedMessageFlags::Contains, 1);
    FTouchWorld F;
    F.Settings->DwellSeconds = 0.0f;
    AActor* Owner = F.Actor();
    UStaticMeshComponent* Mesh = F.Cube(Owner);
    UReachTouchComponent* Touch = F.Touch(Mesh);
    Owner->DispatchBeginPlay();
    FXRHandTrackingState Palm = Hand(FVector(1000.0));
    Palm.HandKeyLocations[static_cast<int32>(EHandKeypoint::Palm)] = FVector(50.5, 0.0, 0.0);
    for (EReachContactPart Part : {EReachContactPart::IndexTip, EReachContactPart::Fingertips, EReachContactPart::WholeHand})
    {
        F.Settings->ContactPart = Part;
        F.Frame(Palm); F.Frame(Palm);
        TestEqual(TEXT("E5: only whole hand includes palm"), Touch->IsTouching(), Part == EReachContactPart::WholeHand);
    }
    // Complex-only geometry distinguishes the two methods (S3 E7).
    UStaticMesh* ComplexMesh = DuplicateObject<UStaticMesh>(Mesh->GetStaticMesh(), GetTransientPackage());
    UBodySetup* Body = ComplexMesh->GetBodySetup();
    Body->CollisionTraceFlag = CTF_UseComplexAsSimple;
    Body->AggGeom.EmptyElements();
    Body->InvalidatePhysicsData();
    Body->CreatePhysicsMeshes();
    Mesh->SetStaticMesh(ComplexMesh);
    F.Settings->ContactMethod = EReachContactMethod::SimpleDistance;
    F.Frame(Hand());
    TestFalse(TEXT("E7: no simple distance on complex-only geometry"), Touch->IsTouching());
    F.Settings->ContactMethod = EReachContactMethod::ComplexOverlap;
    F.Frame(Hand());
    TestTrue(TEXT("E7: complex overlap touches"), Touch->IsTouching());
    F.Frame(Hand(FVector(52.0, 0.0, 0.0)));
    TestFalse(TEXT("E7: outside overlap radius releases"), Touch->IsTouching());
    FXRHandTrackingState Radius = Hand(FVector(52.0, 0.0, 0.0));
    Radius.HandKeyRadii.Init(1.2f, EHandKeypointCount);
    F.Settings->bUseJointRadius = true;
    F.Frame(Radius);
    TestTrue(TEXT("Live joint radius setting touches"), Touch->IsTouching());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachTouchDoorsTest, "ReachStudy.S3.E8_DoorReset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachTouchDoorsTest::RunTest(const FString& Parameters)
{
    AddExpectedMessage(TEXT("Pawn is not ReachHandPawn"), EAutomationExpectedMessageFlags::Contains, 1);
    FTouchWorld F;
    F.Settings->DwellSeconds = 0.0f;
    AActor* A = F.Actor();
    AActor* Child = F.Actor();
    Child->AttachToActor(A, FAttachmentTransformRules::KeepWorldTransform);
    AActor* B = F.Actor();
    AActor* IP = F.Actor();
    UReachTouchComponent* DoorTouch = F.Touch(F.Cube(Child));
    UReachTouchComponent* IPTouch = F.Touch(F.Cube(IP));
    UReachTouchTestObserver* DoorEvents = Observe(DoorTouch);
    UReachTouchTestObserver* IPEvents = Observe(IPTouch);
    AReachDoorVariants* Doors = F.World->SpawnActor<AReachDoorVariants>();
    FReachDoorVariant First; First.Actors.Add(A);
    FReachDoorVariant Second; Second.Actors.Add(B);
    Doors->Variants = {First, Second};
    Doors->DispatchBeginPlay();
    Child->DispatchBeginPlay(); IP->DispatchBeginPlay();
    F.Frame(Hand()); F.Frame(Hand());
    TestTrue(TEXT("E8: Door and IP touch"), DoorTouch->IsTouching() && IPTouch->IsTouching());
    Doors->SelectVariant(1);
    for (UReachTouchComponent* Touch : {DoorTouch, IPTouch})
    {
        TestTrue(TEXT("E8: synchronous reset"), Touch->GetPhase() == EReachTouchPhase::Idle);
        TestNull(TEXT("E8: synchronous overlay clear"), Cast<UMeshComponent>(Touch->GetAttachParent())->GetOverlayMaterial());
    }
    TestEqual(TEXT("E8: Door end once"), DoorEvents->Ends, 1);
    TestEqual(TEXT("E8: IP end once"), IPEvents->Ends, 1);
    F.Frame(Hand());
    TestFalse(TEXT("E8: first IP measurement skipped"), IPTouch->IsTouching());
    F.Frame(Hand());
    TestTrue(TEXT("E8: IP resumes"), IPTouch->IsTouching());
    TestFalse(TEXT("E8: inactive child stays idle"), DoorTouch->IsTouching());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachTouchRegistrationTest, "ReachStudy.S3.E9_InvalidAndDuplicateParents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachTouchRegistrationTest::RunTest(const FString& Parameters)
{
    AddExpectedMessage(TEXT("parent is not a mesh"), EAutomationExpectedMessageFlags::Contains, 1);
    AddExpectedMessage(TEXT("already has a ReachTouchComponent"), EAutomationExpectedMessageFlags::Contains, 1);
    AddExpectedMessage(TEXT("Pawn is not ReachHandPawn"), EAutomationExpectedMessageFlags::Contains, 1);
    FTouchWorld F;
    F.Settings->DwellSeconds = 0.0f;
    AActor* Owner = F.Actor();
    UReachTouchComponent* Invalid = F.Touch(Owner->GetRootComponent());
    UStaticMeshComponent* Mesh = F.Cube(Owner);
    UReachTouchComponent* First = F.Touch(Mesh);
    UReachTouchComponent* Duplicate = F.Touch(Mesh);
    Owner->DispatchBeginPlay();
    F.Frame(Hand()); F.Frame(Hand());
    TestFalse(TEXT("E9: invalid ignored"), Invalid->IsTouching());
    TestTrue(TEXT("E9: first registered touches"), First->IsTouching());
    TestFalse(TEXT("E9: duplicate ignored"), Duplicate->IsTouching());
    Duplicate->DestroyComponent();
    TestNotNull(TEXT("E9: duplicate shutdown leaves first overlay"), Mesh->GetOverlayMaterial());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachTouchSettingsTest, "ReachStudy.S3.E10_SettingsFallbackAndFirst", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachTouchSettingsTest::RunTest(const FString& Parameters)
{
    AddExpectedMessage(TEXT("No ReachTouchSettings"), EAutomationExpectedMessageFlags::Contains, 1);
    AddExpectedMessage(TEXT("Multiple ReachTouchSettings"), EAutomationExpectedMessageFlags::Contains, 1);
    AddExpectedMessage(TEXT("Pawn is not ReachHandPawn"), EAutomationExpectedMessageFlags::Contains, 2);
    for (bool bWithSettings : {false, true})
    {
        FTouchWorld F(bWithSettings);
        if (bWithSettings)
        {
            F.Settings->DwellSeconds = 0.0f;
            F.World->SpawnActor<AReachTouchSettings>()->DwellSeconds = 2.0f;
        }
        AActor* Owner = F.Actor();
        UReachTouchComponent* Touch = F.Touch(F.Cube(Owner));
        Owner->DispatchBeginPlay();
        F.Frame(Hand()); F.Frame(Hand());
        TestEqual(TEXT("E10: first settings or default dwell"), Touch->IsTouching(), bWithSettings);
        for (int32 Frame = 0; Frame < 20; ++Frame) { F.Frame(Hand()); }
        TestTrue(TEXT("E10: fallback also touches"), Touch->IsTouching());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachTouchMutationTest, "ReachStudy.S3.E11_DeferredEvents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachTouchMutationTest::RunTest(const FString& Parameters)
{
    AddExpectedMessage(TEXT("Pawn is not ReachHandPawn"), EAutomationExpectedMessageFlags::Contains, 1);
    FTouchWorld F;
    F.Settings->DwellSeconds = 0.0f;
    AActor* Owner = F.Actor();
    UReachTouchComponent* First = F.Touch(F.Cube(Owner));
    UReachTouchComponent* Second = F.Touch(F.Cube(Owner));
    UReachTouchComponent* Third = F.Touch(F.Cube(Owner));
    UReachTouchTestObserver* Events = Observe(First);
    UReachTouchTestObserver* DeletedEvents = Observe(Second);
    Events->DestroyOnBegin = Second;
    Events->Other = Third;
    Owner->DispatchBeginPlay();
    F.Frame(Hand()); F.Frame(Hand());
    TestTrue(TEXT("E11: all states updated before events"), Events->bOtherWasTouching);
    TestTrue(TEXT("E11: third survives deletion"), Third->IsTouching());
    TestEqual(TEXT("E11: no begin for deleted component"), DeletedEvents->Begins, 0);
    TestEqual(TEXT("E11: no shutdown end"), DeletedEvents->Ends, 0);
    F.Frame(Hand());
    TestTrue(TEXT("E11: following frame safe"), Third->IsTouching());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachTouchBroadphaseTest, "ReachStudy.S3.E12_E13_BoundsAndQueryParity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachTouchBroadphaseTest::RunTest(const FString& Parameters)
{
    FTouchWorld F;
    AActor* Owner = F.Actor();
    UStaticMeshComponent* Mesh = F.Cube(Owner);
    for (bool bLargeSimple : {false, true})
    {
        if (bLargeSimple)
        {
            UStaticMesh* Large = DuplicateObject<UStaticMesh>(Mesh->GetStaticMesh(), GetTransientPackage());
            UBodySetup* Body = Large->GetBodySetup();
            Body->CollisionTraceFlag = CTF_UseSimpleAndComplex;
            Body->AggGeom.EmptyElements();
            FKBoxElem Box; Box.X = 200.0f; Box.Y = 200.0f; Box.Z = 200.0f;
            Body->AggGeom.BoxElems.Add(Box);
            Body->InvalidatePhysicsData(); Body->CreatePhysicsMeshes();
            Mesh->SetStaticMesh(Large);
        }
        for (bool bRadius : {false, true})
        {
            for (double X : {-110.0, -102.3, -101.5, -100.5, -52.3, -51.5, -50.5, 0.0, 50.5, 51.5, 52.3, 100.5, 101.5, 102.3, 110.0})
            {
                for (double Y : {0.0, 49.0, 101.0, 110.0})
                {
                    for (double Z : {0.0, 51.0, 110.0})
                    {
                        const FReachHandSample Sample = ReachContact::ReadSample(Hand(FVector(X, Y, Z)), EReachContactPart::IndexTip);
                        FReachHandSample WithRadius = Sample;
                        WithRadius.Joints[0].RadiusMm = 8.0f;
                        const FReachContactQueryResult Direct = ReachContact::QueryComponent(Mesh, WithRadius, 10.0f, 5.0f, bRadius);
                        const FReachContactQueryResult Filtered = ReachContact::IsWithinContactBounds(Mesh, WithRadius, 10.0f, 5.0f)
                            ? Direct : FReachContactQueryResult{};
                        const FReachContactQueryResult Actor = ReachContact::Query(Owner, WithRadius, 10.0f, 5.0f, bRadius);
                        const FString Case = FString::Printf(TEXT("large=%d radius=%d point=(%.17g,%.17g,%.17g) distance=%.9g"), bLargeSimple, bRadius, X, Y, Z, Direct.DistanceMm);
                        TestTrue(FString::Printf(TEXT("E12: simple unchanged %s"), *Case), Direct.Simple == Filtered.Simple);
                        TestTrue(FString::Printf(TEXT("E12: complex unchanged %s"), *Case), Direct.Complex == Filtered.Complex);
                        TestEqual(TEXT("E13: distance"), Actor.DistanceMm, Direct.DistanceMm);
                        TestEqual(TEXT("E13: joint radius"), Actor.JointRadiusMm, Direct.JointRadiusMm);
                        TestTrue(TEXT("E13: nearest joint"), Actor.NearestJoint == Direct.NearestJoint);
                        TestEqual(TEXT("E13: collision"), Actor.bHasCollision, Direct.bHasCollision);
                        TestTrue(TEXT("E13: simple and complex"), Actor.Simple == Direct.Simple && Actor.Complex == Direct.Complex);
                    }
                }
            }
        }
        if (bLargeSimple)
        {
            const auto Sample = ReachContact::ReadSample(Hand(FVector(100.5, 0.0, 0.0)), EReachContactPart::IndexTip);
            TestTrue(TEXT("E12: includes body outside rendering bounds"), ReachContact::IsWithinContactBounds(Mesh, Sample, 10.0f, 5.0f));
            TestTrue(TEXT("E12: oversized simple body really enters"), ReachContact::QueryComponent(Mesh, Sample, 10.0f, 5.0f, false).Simple == EReachContactCondition::Enter);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachTouchPawnWorldTest, "ReachStudy.S3.E14_E16_PawnAndWorld", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachTouchPawnWorldTest::RunTest(const FString& Parameters)
{
    AddExpectedMessage(TEXT("Pawn is not ReachHandPawn"), EAutomationExpectedMessageFlags::Contains, 1);
    TGuardValue<bool> RestoreMessages(GAreScreenMessagesEnabled, true);
    {
        FTouchWorld F;
        F.Settings->DwellSeconds = 0.0f;
        APlayerController* PC = F.World->SpawnActor<APlayerController>();
        PC->Possess(F.World->SpawnActor<APawn>());
        AActor* Owner = F.Actor();
        UReachTouchComponent* Touch = F.Touch(F.Cube(Owner));
        F.World->BeginPlay();
        if (!Owner->HasActorBegunPlay()) { Owner->DispatchBeginPlay(); }
        F.Frame(Hand()); F.Frame(Hand());
        TestTrue(TEXT("E14: plain Pawn still touches"), Touch->IsTouching());
        TestFalse(TEXT("E14: screen messages remain off"), GAreScreenMessagesEnabled);
    }
    {
        FTouchWorld F(true, EWorldType::Editor);
        TestNull(TEXT("E16: no Editor subsystem"), F.System);
        AActor* Owner = F.Actor();
        UStaticMeshComponent* Mesh = F.Cube(Owner);
        F.Touch(Mesh);
        TestNull(TEXT("E16: no overlay"), Mesh->GetOverlayMaterial());
    }
    TestEqual(TEXT("D10: product Pawn"), GetDefault<AReachGameMode>()->DefaultPawnClass.Get(), AReachHandPawn::StaticClass());
    return true;
}
#endif

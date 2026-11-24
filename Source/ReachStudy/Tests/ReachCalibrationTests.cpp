#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ReachCalibrationTestObserver.h"
#include "ReachCalibrationGameMode.h"
#include "ReachGameMode.h"
#include "Hand/ReachHandPawn.h"
#include "Components/SceneComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "HeadMountedDisplayTypes.h"
#include "Templates/UnrealTemplate.h"
#include "VRCalAlignmentComponent.h"
#include "VRCalAlignPointComponent.h"

namespace
{
struct FCalibrationWorld
{
    UWorld* World;

    explicit FCalibrationWorld(TSubclassOf<AGameModeBase> GameMode = nullptr,
        EWorldType::Type Type = EWorldType::Game)
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(true).ShouldSimulatePhysics(false)
            .EnableTraceCollision(true).CreateNavigation(false).CreateAISystem(false);
        World = UWorld::CreateWorld(Type, false, NAME_None, nullptr, true,
            ERHIFeatureLevel::Num, &Values);
        GEngine->CreateNewWorldContext(Type).SetCurrentWorld(World);
        if (GameMode)
        {
            World->SetGameInstance(NewObject<UGameInstance>(GEngine));
            World->GetWorldSettings()->DefaultGameMode = GameMode;
            World->SetGameMode(FURL());
        }
        World->InitializeActorsForPlay(FURL());
    }

    ~FCalibrationWorld()
    {
        // Mark teardown before actor EndPlay can respawn subsystem helpers (S1 E1-E4).
        World->EndPlay(EEndPlayReason::Quit);
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    }
};

struct FCalibrationPawnFixture
{
    FCalibrationWorld Fixture;
    AActor* Car;
    AReachHandPawn* Pawn;
    UVRCalAlignmentComponent* Alignment;
    FVRCalSavedAlignment Saved;
    const FVector2D PlayArea = FVector2D(300.0, 400.0);

    FCalibrationPawnFixture()
    {
        Car = Fixture.World->SpawnActor<AActor>();
        USceneComponent* Root = NewObject<USceneComponent>(Car);
        Car->AddInstanceComponent(Root);
        Car->SetRootComponent(Root);
        Root->RegisterComponent();
        Alignment = NewObject<UVRCalAlignmentComponent>(Car);
        Car->AddInstanceComponent(Alignment);
        Alignment->RegisterComponent();
        // Synthetic square: Stage points rotated 90 degrees and translated (S1 E5).
        const FVector Local[] = {{120, -80, 50}, {120, -40, 50}, {60, -40, 50}, {60, -80, 50}};
        const FVector Stage[] = {{0, 0, 50}, {40, 0, 50}, {40, 60, 50}, {0, 60, 50}};
        for (int32 Index = 0; Index < 4; ++Index)
        {
            UVRCalAlignPointComponent* Point = NewObject<UVRCalAlignPointComponent>(Car);
            Car->AddInstanceComponent(Point);
            Point->SetupAttachment(Root);
            Point->Order = Index + 1;
            Point->SetRelativeLocation(Local[Index]);
            Point->RegisterComponent();
            Saved.Points.Add({Index + 1, Stage[Index], 0.0, 10});
        }
        Saved.TrackingOrigin = EHMDTrackingOrigin::Stage;
        Saved.PlayAreaCm = PlayArea;
        Saved.SavedAt = TEXT("2026-01-01T00:00:00Z");
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Pawn = Fixture.World->SpawnActor<AReachHandPawn>(AReachHandPawn::StaticClass(),
            FVector(17, 23, 29), FRotator::ZeroRotator, Spawn);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachS1StartPlayTest, "ReachStudy.S1.E1_StageScaleBeforeBeginPlay", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachS1StartPlayTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> RestoreMessages(GAreScreenMessagesEnabled, GAreScreenMessagesEnabled);
    FCalibrationWorld Fixture(AReachGameMode::StaticClass());
    if (!TestNotNull(TEXT("E1: world has ReachGameMode"), Fixture.World->GetAuthGameMode<AReachGameMode>())) { return false; }
    Fixture.World->GetWorldSettings()->WorldToMeters = 50.0f;
    AReachCalibrationTestObserver* Observer = Fixture.World->SpawnActor<AReachCalibrationTestObserver>();
    Fixture.World->BeginPlay();
    TestTrue(TEXT("E1: actor received BeginPlay"), Observer->HasActorBegunPlay());
    TestEqual(TEXT("E1: actor BeginPlay sees cm scale"), Observer->BeginPlayWorldToMeters, 100.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachS1CalibrationModeTest, "ReachStudy.S1.E2_CalibrationModeInheritsSetup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachS1CalibrationModeTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> RestoreMessages(GAreScreenMessagesEnabled, GAreScreenMessagesEnabled);
    TestTrue(TEXT("E2: calibration mode inherits ReachGameMode"), AReachCalibrationGameMode::StaticClass()->IsChildOf(AReachGameMode::StaticClass()));
    FCalibrationWorld Fixture(AReachCalibrationGameMode::StaticClass());
    AReachCalibrationGameMode* Mode = Fixture.World->GetAuthGameMode<AReachCalibrationGameMode>();
    if (!TestNotNull(TEXT("E2: calibration mode is authoritative"), Mode)) { return false; }
    TestTrue(TEXT("E2: default pawn is ReachHandPawn"), Mode->DefaultPawnClass == AReachHandPawn::StaticClass());
    Fixture.World->GetWorldSettings()->WorldToMeters = 50.0f;
    AReachCalibrationTestObserver* Observer = Fixture.World->SpawnActor<AReachCalibrationTestObserver>();
    Fixture.World->BeginPlay();
    TestTrue(TEXT("E2: actor received BeginPlay"), Observer->HasActorBegunPlay());
    TestEqual(TEXT("E2: actor BeginPlay sees cm scale"), Observer->BeginPlayWorldToMeters, 100.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachS1CalibrationMessagesTest, "ReachStudy.S1.E3_CalibrationKeepsMessages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachS1CalibrationMessagesTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> RestoreMessages(GAreScreenMessagesEnabled, GAreScreenMessagesEnabled);
    for (EWorldType::Type Type : {EWorldType::Game, EWorldType::PIE})
    {
        for (bool bInitiallyEnabled : {true, false})
        {
            GAreScreenMessagesEnabled = bInitiallyEnabled;
            {
                FCalibrationWorld Fixture(AReachCalibrationGameMode::StaticClass(), Type);
                if (!TestNotNull(TEXT("E3: calibration mode exists"), Fixture.World->GetAuthGameMode<AReachCalibrationGameMode>())) { return false; }
                Fixture.World->BeginPlay();
                TestEqual(TEXT("E3: BeginPlay preserves initial messages"), GAreScreenMessagesEnabled, bInitiallyEnabled);
            }
            TestEqual(TEXT("E3: teardown preserves initial messages"), GAreScreenMessagesEnabled, bInitiallyEnabled);
        }
    }
    GAreScreenMessagesEnabled = true;
    {
        FCalibrationWorld Fixture(AReachCalibrationGameMode::StaticClass());
        Fixture.World->BeginPlay();
        // Catch teardown restoring a value this world never changed (S1 D5).
        GAreScreenMessagesEnabled = false;
    }
    TestFalse(TEXT("E3: teardown does not restore after an external change"), GAreScreenMessagesEnabled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachS1MainMessagesTest, "ReachStudy.S1.E4_MainAndNoGameModeRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachS1MainMessagesTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> RestoreMessages(GAreScreenMessagesEnabled, GAreScreenMessagesEnabled);
    for (EWorldType::Type Type : {EWorldType::Game, EWorldType::PIE})
    {
        for (bool bUseGameMode : {true, false})
        {
            for (bool bInitiallyEnabled : {true, false})
            {
                GAreScreenMessagesEnabled = bInitiallyEnabled;
                {
                    FCalibrationWorld Fixture(bUseGameMode ? AReachGameMode::StaticClass() : nullptr, Type);
                    TestEqual(TEXT("E4: expected GameMode presence"), Fixture.World->GetAuthGameMode() != nullptr, bUseGameMode);
                    Fixture.World->BeginPlay();
                    TestFalse(TEXT("E4: main and no-GameMode worlds disable messages"), GAreScreenMessagesEnabled);
                }
                TestEqual(TEXT("E4: teardown restores initial messages"), GAreScreenMessagesEnabled, bInitiallyEnabled);
            }
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachS1KitPawnTest, "ReachStudy.S1.E5_KitAppliesToHandPawn", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachS1KitPawnTest::RunTest(const FString& Parameters)
{
    FCalibrationPawnFixture Fixture;
    Fixture.Alignment->InitializeAlignment(Fixture.Pawn, &Fixture.Saved, EHMDTrackingOrigin::Stage, Fixture.PlayArea);
    TestTrue(TEXT("E5: kit applied alignment"), Fixture.Alignment->State == EVRCalAlignmentState::Applied);
    TestTrue(TEXT("E5: kit attached hand pawn to car"), Fixture.Pawn->GetAttachParentActor() == Fixture.Car);
    const FTransform Relative = Fixture.Pawn->GetRootComponent()->GetRelativeTransform();
    TestTrue(TEXT("E5: fitted relative location within 0.1 cm"), Relative.GetLocation().Equals(FVector(120, -80, 0), 0.1));
    TestTrue(TEXT("E5: fitted relative yaw is 90 degrees"), Relative.GetRotation().Equals(FRotator(0, 90, 0).Quaternion(), 0.001));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachS1NonStageTest, "ReachStudy.S1.E6_NonStageLeavesPawnAlone", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachS1NonStageTest::RunTest(const FString& Parameters)
{
    FCalibrationPawnFixture Fixture;
    const FTransform Before = Fixture.Pawn->GetActorTransform();
    Fixture.Alignment->InitializeAlignment(Fixture.Pawn, &Fixture.Saved, EHMDTrackingOrigin::Local, Fixture.PlayArea);
    TestTrue(TEXT("E6: non-Stage needs check"), Fixture.Alignment->State == EVRCalAlignmentState::NeedsCheck);
    TestNull(TEXT("E6: pawn remains unattached"), Fixture.Pawn->GetAttachParentActor());
    TestTrue(TEXT("E6: pawn transform unchanged"), Fixture.Pawn->GetActorTransform().Equals(Before, 0.001));
    return true;
}
#endif

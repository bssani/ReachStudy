#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Screen/ReachScreenMessageSubsystem.h"
#include "Development/ReachContactProbe.h"
#include "Door/ReachDoorPanel.h"
#include "Door/ReachDoorPanelWidget.h"
#include "Door/ReachDoorVariants.h"
#include "Components/HorizontalBox.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "IsdkInteractableWidgetComponent.h"
#include "Components/WidgetComponent.h"
#include "Templates/UnrealTemplate.h"

namespace
{
struct FScreenMessageWorld
{
    UWorld* World;

    explicit FScreenMessageWorld(EWorldType::Type Type = EWorldType::Game)
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(true).ShouldSimulatePhysics(false)
            .EnableTraceCollision(true).CreateNavigation(false).CreateAISystem(false);
        World = UWorld::CreateWorld(Type, false, NAME_None, nullptr, true,
            ERHIFeatureLevel::Num, &Values);
        GEngine->CreateNewWorldContext(Type).SetCurrentWorld(World);
        if (Type != EWorldType::Editor) { World->InitializeActorsForPlay(FURL()); }
    }

    ~FScreenMessageWorld()
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->HasActorBegunPlay()) { It->RouteEndPlay(EEndPlayReason::Quit); }
        }
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachScreenMessageLifetimeTest, "ReachStudy.S4b.E1_GameAndPIERestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachScreenMessageLifetimeTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> RestoreMessages(GAreScreenMessagesEnabled, GAreScreenMessagesEnabled);
    for (EWorldType::Type Type : {EWorldType::Game, EWorldType::PIE})
    {
        for (bool bInitiallyEnabled : {true, false})
        {
            GAreScreenMessagesEnabled = bInitiallyEnabled;
            {
                FScreenMessageWorld Fixture(Type);
                UReachScreenMessageSubsystem* Subsystem = Fixture.World->GetSubsystem<UReachScreenMessageSubsystem>();
                if (!TestNotNull(TEXT("Game and PIE create subsystem"), Subsystem)) { return false; }
                TestEqual(TEXT("Before BeginPlay leaves messages unchanged"), GAreScreenMessagesEnabled, bInitiallyEnabled);
                Fixture.World->BeginPlay();
                TestFalse(TEXT("E1: BeginPlay disables messages"), GAreScreenMessagesEnabled);
            }
            TestEqual(TEXT("E1: world teardown restores initial setting"), GAreScreenMessagesEnabled, bInitiallyEnabled);
        }
    }
    GAreScreenMessagesEnabled = true;
    {
        FScreenMessageWorld Fixture;
        GAreScreenMessagesEnabled = false;
    }
    TestFalse(TEXT("World that never began play does not restore a setting"), GAreScreenMessagesEnabled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachScreenMessageEditorTest, "ReachStudy.S4b.E2_EditorUnaffected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachScreenMessageEditorTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> RestoreMessages(GAreScreenMessagesEnabled, GAreScreenMessagesEnabled);
    for (bool bInitiallyEnabled : {true, false})
    {
        GAreScreenMessagesEnabled = bInitiallyEnabled;
        {
            FScreenMessageWorld Fixture(EWorldType::Editor);
            TestNull(TEXT("E2: no Editor subsystem"), Fixture.World->GetSubsystem<UReachScreenMessageSubsystem>());
            TestEqual(TEXT("E2: Editor creation leaves setting unchanged"), GAreScreenMessagesEnabled, bInitiallyEnabled);
        }
        TestEqual(TEXT("E2: Editor teardown leaves setting unchanged"), GAreScreenMessagesEnabled, bInitiallyEnabled);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachScreenMessageProbeTest, "ReachStudy.S4b.E3_ProbeWithoutMessages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachScreenMessageProbeTest::RunTest(const FString& Parameters)
{
    TGuardValue<bool> RestoreMessages(GAreScreenMessagesEnabled, true);
    FScreenMessageWorld Fixture;
    AActor* Target = Fixture.World->SpawnActor<AActor>();
    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Target);
    Target->AddInstanceComponent(Mesh);
    Target->SetRootComponent(Mesh);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->RegisterComponent();
    AReachContactProbe* Probe = Fixture.World->SpawnActor<AReachContactProbe>();
    Probe->Targets.Add(Target);
    Probe->ContactMethod = EReachContactMethod::SimpleDistance;
    Probe->DwellSeconds = 0.0f;
    FReachHandSample Hand;
    Hand.bTracked = true;
    Hand.Joints.Add({EHandKeypoint::IndexTip, FVector(50.5, 0.0, 0.0), 0.0f});
    for (int32 Frame = 0; Frame < 3; ++Frame)
    {
        Probe->ProcessFrame(Hand, {}, 0.01f);
        TestFalse(TEXT("E3: ProcessFrame emits no screen message"), GEngine->OnScreenDebugMessageExists(Probe->GetUniqueID()));
    }
    TestTrue(TEXT("E3: S2 contact still touches"), Probe->GetResults()[0].Phase == EReachContactPhase::Touching);
    TestNotNull(TEXT("E3: touch still applies overlay"), Mesh->GetOverlayMaterial());
    Probe->Tick(0.01f);
    TestFalse(TEXT("E3: Tick emits no screen message even when enabled"), GEngine->OnScreenDebugMessageExists(Probe->GetUniqueID()));
    Hand.Joints[0].WorldPosition = FVector(60.0, 0.0, 0.0);
    Probe->ProcessFrame(Hand, {}, 0.01f);
    Probe->ProcessFrame(Hand, {}, 0.01f);
    TestTrue(TEXT("E3: S2 contact still releases"), Probe->GetResults()[0].Phase == EReachContactPhase::Idle);
    TestNull(TEXT("E3: release still clears overlay"), Mesh->GetOverlayMaterial());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachScreenMessageEmptyPanelTest, "ReachStudy.S4b.E4_EmptyPanel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachScreenMessageEmptyPanelTest::RunTest(const FString& Parameters)
{
    FScreenMessageWorld Fixture;
    AReachDoorPanel* Panel = Fixture.World->SpawnActor<AReachDoorPanel>();
    AddExpectedMessage(TEXT("No door variants"), EAutomationExpectedMessageFlags::Contains, 2);
    Panel->DispatchBeginPlay();
    UReachDoorPanelWidget* Widget = Cast<UReachDoorPanelWidget>(Panel->InteractableWidget->GetWidget()->GetUserWidgetObject());
    if (!TestNotNull(TEXT("E4: panel widget exists"), Widget)) { return false; }
    const TSharedRef<SWidget> Slate = Widget->TakeWidget();
    UHorizontalBox* Row = Cast<UHorizontalBox>(Widget->GetRootWidget());
    if (!TestNotNull(TEXT("E4: panel row exists"), Row)) { return false; }
    TestEqual(TEXT("E4: no buttons or text without connection"), Row->GetChildrenCount(), 0);
    AReachDoorVariants* EmptyDoors = Fixture.World->SpawnActor<AReachDoorVariants>();
    Widget->SetDoorVariants(EmptyDoors);
    TestEqual(TEXT("E4: no buttons or text with empty variants"), Row->GetChildrenCount(), 0);
    AReachDoorPanel* EmptyPanel = Fixture.World->SpawnActor<AReachDoorPanel>();
    EmptyPanel->DoorVariants = EmptyDoors;
    EmptyPanel->DispatchBeginPlay();
    return true;
}
#endif

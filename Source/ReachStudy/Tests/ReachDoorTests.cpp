#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/ReachDoorTestObserver.h"
#include "Door/ReachDoorVariants.h"
#include "Door/ReachDoorPanel.h"
#include "Door/ReachDoorPanelWidget.h"
#include "Development/ReachContactProbe.h"
#include "Hand/ReachHandPawn.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Interaction/IsdkGrabberComponent.h"
#include "IsdkInteractableWidgetComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Rig/IsdkGrabInteractionRigComponent.h"
#include "Rig/IsdkPokeInteractionRigComponent.h"
#include "Rig/IsdkPokeLimiterRigModifier.h"
#include "Rig/IsdkRayInteractionRigComponent.h"
#include "UObject/UnrealType.h"

namespace
{
struct FDoorWorld
{
    UWorld* World;
    AReachDoorVariants* Doors;
    AActor* A;
    AActor* B;
    AActor* IP;

    FDoorWorld()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(true).ShouldSimulatePhysics(false)
            .EnableTraceCollision(true).CreateNavigation(false).CreateAISystem(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
            ERHIFeatureLevel::Num, &Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        World->InitializeActorsForPlay(FURL());
        A = Actor();
        B = Actor();
        IP = Actor();
        Doors = World->SpawnActor<AReachDoorVariants>();
        FReachDoorVariant First;
        First.Name = FText::FromString(TEXT("Door A"));
        First.Actors.Add(A);
        FReachDoorVariant Second;
        Second.Name = FText::FromString(TEXT("Door B"));
        Second.Actors.Add(B);
        Doors->Variants = {First, Second};
    }

    AActor* Actor()
    {
        AActor* Result = World->SpawnActor<AActor>();
        USceneComponent* Root = NewObject<USceneComponent>(Result);
        Result->AddInstanceComponent(Root);
        Result->SetRootComponent(Root);
        Root->RegisterComponent();
        return Result;
    }

    UStaticMeshComponent* Cube(AActor* Owner)
    {
        UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
        Owner->AddInstanceComponent(Mesh);
        Mesh->SetupAttachment(Owner->GetRootComponent());
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        Mesh->RegisterComponent();
        return Mesh;
    }

    ~FDoorWorld()
    {
        // DispatchBeginPlay needs matching actor/component shutdown before subsystem teardown.
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->HasActorBegunPlay()) { It->RouteEndPlay(EEndPlayReason::Quit); }
        }
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    }
};

FReachHandSample SurfacePoint()
{
    FReachHandSample Hand;
    Hand.bTracked = true;
    Hand.Joints.Add({EHandKeypoint::IndexTip, FVector(50.5, 0.0, 0.0), 0.0f});
    return Hand;
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachDoorSelectionTest, "ReachStudy.S4.E1_E4_Selection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachDoorSelectionTest::RunTest(const FString& Parameters)
{
    FDoorWorld F;
    AActor* A2 = F.Actor();
    AActor* B2 = F.Actor();
    F.Doors->Variants[0].Actors.Add(A2);
    F.Doors->Variants[1].Actors.Add(B2);
    UReachDoorTestObserver* Events = NewObject<UReachDoorTestObserver>();
    F.Doors->OnVariantChanged.AddDynamic(Events, &UReachDoorTestObserver::Record);
    F.Doors->DispatchBeginPlay();
    TestEqual(TEXT("E1: initial selection"), F.Doors->GetActiveVariant(), 0);
    TestEqual(TEXT("E1: no startup event"), Events->Count, 0);
    for (AActor* Actor : {F.A, A2, F.IP})
    {
        TestFalse(TEXT("E1: active and unrelated visible"), Actor->IsHidden());
        TestTrue(TEXT("E1: active and unrelated collision"), Actor->GetActorEnableCollision());
    }
    for (AActor* Actor : {F.B, B2})
    {
        TestTrue(TEXT("E1: inactive hidden"), Actor->IsHidden());
        TestFalse(TEXT("E1: inactive collision off"), Actor->GetActorEnableCollision());
    }
    F.Doors->SelectVariant(1);
    TestTrue(TEXT("E2: A hidden and off"), F.A->IsHidden() && !F.A->GetActorEnableCollision());
    TestTrue(TEXT("E2: B restored"), !F.B->IsHidden() && F.B->GetActorEnableCollision());
    TestEqual(TEXT("E2: one change event"), Events->Count, 1);
    TestEqual(TEXT("E2: event index"), Events->LastIndex, 1);
    F.Doors->SelectVariant(1);
    TestEqual(TEXT("E3: repeated selection has no event"), Events->Count, 1);
    AddExpectedMessage(TEXT("Invalid door variant index"), EAutomationExpectedMessageFlags::Contains, 2);
    F.Doors->SelectVariant(-1);
    F.Doors->SelectVariant(2);
    TestEqual(TEXT("E4: invalid selection keeps index"), F.Doors->GetActiveVariant(), 1);
    TestEqual(TEXT("E4: invalid selection has no event"), Events->Count, 1);
    TestTrue(TEXT("E4: B still restored"), !F.B->IsHidden() && F.B->GetActorEnableCollision());
    TestTrue(TEXT("IP is always active"), F.Doors->IsActorActive(F.IP));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachDoorRestoreTest, "ReachStudy.S4.E5_E7_E17_MembershipAndRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachDoorRestoreTest::RunTest(const FString& Parameters)
{
    for (bool bInclude : {true, false})
    {
        FDoorWorld F;
        F.Doors->bIncludeAttachedActors = bInclude;
        AActor* Child = F.Actor();
        AActor* Grandchild = F.Actor();
        Child->AttachToActor(F.A, FAttachmentTransformRules::KeepWorldTransform);
        Grandchild->AttachToActor(Child, FAttachmentTransformRules::KeepWorldTransform);
        AActor* Shared = F.Actor();
        F.Doors->Variants[0].Actors.Add(Shared);
        F.Doors->Variants[1].Actors.Add(Shared);
        F.Doors->Variants[0].Actors.Add(nullptr);
        AActor* Deleted = F.Actor();
        F.Doors->Variants[1].Actors.Add(Deleted);
        Deleted->Destroy();
        F.B->SetActorHiddenInGame(true);
        F.B->SetActorEnableCollision(false);
        UStaticMeshComponent* Mesh = F.Cube(F.A);
        const auto Materials = Mesh->GetMaterials();
        UStaticMesh* OriginalMesh = Mesh->GetStaticMesh();
        const FTransform Transform = Mesh->GetComponentTransform();
        const FCollisionResponseContainer Responses = Mesh->GetCollisionResponseToChannels();
        F.Doors->DispatchBeginPlay();
        F.Doors->SelectVariant(1);
        TestEqual(TEXT("E5: grandchild hidden only with recursion"), Grandchild->IsHidden(), bInclude);
        TestEqual(TEXT("E5: grandchild collision follows recursion"), Grandchild->GetActorEnableCollision(), !bInclude);
        TestEqual(TEXT("E5: membership follows recursion"), F.Doors->IsActorActive(Grandchild), !bInclude);
        TestTrue(TEXT("E6: shared actor remains original"), !Shared->IsHidden() && Shared->GetActorEnableCollision());
        TestTrue(TEXT("E6: shared membership active"), F.Doors->IsActorActive(Shared));
        TestTrue(TEXT("E7: originally hidden B remains off when active"), F.B->IsHidden() && !F.B->GetActorEnableCollision());
        TestTrue(TEXT("Component collision setting retained while owner off"), Mesh->GetBodyInstance()->GetCollisionEnabled(false) == ECollisionEnabled::QueryOnly);
        F.Doors->SelectVariant(0);
        TestTrue(TEXT("E7: original B retained when inactive"), F.B->IsHidden() && !F.B->GetActorEnableCollision());
        TestTrue(TEXT("Component materials retained"), Mesh->GetMaterials() == Materials);
        TestTrue(TEXT("Component mesh retained"), Mesh->GetStaticMesh() == OriginalMesh);
        TestTrue(TEXT("Component transform retained"), Mesh->GetComponentTransform().Equals(Transform));
        TestTrue(TEXT("Component responses retained"), Mesh->GetCollisionResponseToChannels() == Responses);
        F.Doors->SelectVariant(1);
        Shared->Destroy();
        F.Doors->SelectVariant(0);
        F.Doors->SelectVariant(1);
        F.Doors->RouteEndPlay(EEndPlayReason::Destroyed);
        TestTrue(TEXT("E7: EndPlay restores A"), !F.A->IsHidden() && F.A->GetActorEnableCollision());
        TestTrue(TEXT("E7: EndPlay restores hidden B"), F.B->IsHidden() && !F.B->GetActorEnableCollision());
        TestTrue(TEXT("E7: EndPlay restores grandchild"), !Grandchild->IsHidden() && Grandchild->GetActorEnableCollision());
        TestTrue(TEXT("IP unchanged through shutdown"), !F.IP->IsHidden() && F.IP->GetActorEnableCollision());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachDoorProbeTest, "ReachStudy.S4.E8_E10_ProbeResetAndInactive", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachDoorProbeTest::RunTest(const FString& Parameters)
{
    FDoorWorld F;
    UStaticMeshComponent* AMesh = F.Cube(F.A);
    UStaticMeshComponent* BMesh = F.Cube(F.B);
    UStaticMeshComponent* IPMesh = F.Cube(F.IP);
    F.Doors->DispatchBeginPlay();
    const FReachContactQueryResult Query = ReachContact::Query(F.B, SurfacePoint(), 10.0f, 5.0f, false);
    TestFalse(TEXT("E8: inactive has no collision"), Query.bHasCollision);
    TestTrue(TEXT("E8: neither method enters or holds"), Query.Simple == EReachContactCondition::None && Query.Complex == EReachContactCondition::None);
    AReachContactProbe* Probe = F.World->SpawnActor<AReachContactProbe>();
    Probe->Targets = {F.A, F.B, F.IP};
    Probe->DwellSeconds = 0.0f;
    Probe->DispatchBeginPlay();
    TestTrue(TEXT("Automatic first-world door connection"), Probe->DoorVariants == F.Doors);
    Probe->ProcessFrame(SurfacePoint(), {}, 0.01f);
    Probe->ProcessFrame(SurfacePoint(), {}, 0.01f);
    TestTrue(TEXT("E9: A touching before switch"), Probe->GetResults()[0].Phase == EReachContactPhase::Touching);
    TestNotNull(TEXT("E9: A overlay before switch"), AMesh->GetOverlayMaterial());
    TestNotNull(TEXT("E9: IP overlay before switch"), IPMesh->GetOverlayMaterial());
    TestTrue(TEXT("E10: B inactive and Idle"), Probe->GetResults()[1].bInactive && Probe->GetResults()[1].Phase == EReachContactPhase::Idle);
    TestNull(TEXT("E10: no inactive overlay"), BMesh->GetOverlayMaterial());
    TestTrue(TEXT("Door name in header"), Probe->BuildReadout(SurfacePoint(), {}).Contains(TEXT("door Door A")));
    TestTrue(TEXT("E10: inactive row text"), Probe->BuildReadout(SurfacePoint(), {}).Contains(F.B->GetName() + TEXT(" inactive")));
    F.Doors->SelectVariant(0);
    TestNotNull(TEXT("E3: same selection does not reset contacts"), AMesh->GetOverlayMaterial());
    F.Doors->SelectVariant(1);
    TestNull(TEXT("E9: A cleared synchronously"), AMesh->GetOverlayMaterial());
    TestNull(TEXT("E9: IP cleared synchronously"), IPMesh->GetOverlayMaterial());
    TestEqual(TEXT("E9: old results cleared synchronously"), Probe->GetResults().Num(), 0);
    Probe->ProcessFrame(SurfacePoint(), {}, 0.01f);
    for (const FReachProbeResult& Result : Probe->GetResults())
    {
        TestTrue(TEXT("E9: fresh tracking frame Idle for every target"), Result.Phase == EReachContactPhase::Idle);
    }
    TestTrue(TEXT("E10: A now inactive"), Probe->GetResults()[0].bInactive);
    Probe->ProcessFrame(SurfacePoint(), {}, 0.01f);
    TestTrue(TEXT("New active B touches after fresh measurement"), Probe->GetResults()[1].Phase == EReachContactPhase::Touching);
    TestNull(TEXT("E10: A never regains overlay"), AMesh->GetOverlayMaterial());
    TestNotNull(TEXT("New active B overlay"), BMesh->GetOverlayMaterial());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachDoorAbsentTest, "ReachStudy.S4.E11_NoDoorProbe", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachDoorAbsentTest::RunTest(const FString& Parameters)
{
    FDoorWorld F;
    F.Doors->Destroy();
    UStaticMeshComponent* Mesh = F.Cube(F.IP);
    AReachContactProbe* Probe = F.World->SpawnActor<AReachContactProbe>();
    Probe->Targets.Add(F.IP);
    Probe->DwellSeconds = 0.0f;
    Probe->DispatchBeginPlay();
    Probe->ProcessFrame(SurfacePoint(), {}, 0.01f);
    Probe->ProcessFrame(SurfacePoint(), {}, 0.01f);
    TestTrue(TEXT("E11: no door retains S2 touching"), Probe->GetResults()[0].Phase == EReachContactPhase::Touching);
    TestNotNull(TEXT("E11: no door retains overlay"), Mesh->GetOverlayMaterial());
    TestFalse(TEXT("E11: no door header"), Probe->BuildReadout(SurfacePoint(), {}).Contains(TEXT("door ")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachDoorWidgetTest, "ReachStudy.S4.E12_E14_KeyboardAndWidget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachDoorWidgetTest::RunTest(const FString& Parameters)
{
    FDoorWorld F;
    F.Doors->DispatchBeginPlay();
    UReachDoorTestObserver* Events = NewObject<UReachDoorTestObserver>();
    F.Doors->OnVariantChanged.AddDynamic(Events, &UReachDoorTestObserver::Record);
    UReachDoorPanelWidget* Widget = CreateWidget<UReachDoorPanelWidget>(F.World);
    if (!TestNotNull(TEXT("Widget created"), Widget)) { return false; }
    Widget->SetDoorVariants(F.Doors);
    const TSharedRef<SWidget> Slate = Widget->TakeWidget();
    UHorizontalBox* Row = Cast<UHorizontalBox>(Widget->GetRootWidget());
    if (!TestNotNull(TEXT("Horizontal button row"), Row)) { return false; }
    TestEqual(TEXT("One button per variant"), Row->GetChildrenCount(), 2);
    UButton* Button0 = Cast<UButton>(Row->GetChildAt(0));
    UButton* Button1 = Cast<UButton>(Row->GetChildAt(1));
    if (!TestNotNull(TEXT("First button"), Button0) || !TestNotNull(TEXT("Second button"), Button1)) { return false; }
    TestTrue(TEXT("Button label from variant"), CastChecked<UTextBlock>(Button1->GetContent())->GetText().ToString() == TEXT("Door B"));
    Button1->OnClicked.Broadcast();
    TestEqual(TEXT("E14: button selects absolute index"), F.Doors->GetActiveVariant(), 1);
    TestTrue(TEXT("E14: active button color"), Button1->GetBackgroundColor() == Widget->ActiveColor);
    TestTrue(TEXT("E14: idle button color"), Button0->GetBackgroundColor() == Widget->IdleColor);
    Button1->OnClicked.Broadcast();
    TestEqual(TEXT("Same button no duplicate event"), Events->Count, 1);
    F.Doors->HandleKey(EKeys::C);
    F.Doors->HandleKey(EKeys::V);
    TestEqual(TEXT("E12: missing C/V variants ignored"), Events->Count, 1);
    F.Doors->HandleKey(EKeys::Z);
    TestEqual(TEXT("E12: Z selects zero"), F.Doors->GetActiveVariant(), 0);
    TestTrue(TEXT("Keyboard event refreshes color"), Button0->GetBackgroundColor() == Widget->ActiveColor);
    F.Doors->HandleKey(EKeys::X);
    Button0->OnClicked.Broadcast();
    TestEqual(TEXT("E13: last input wins"), F.Doors->GetActiveVariant(), 0);
    TestEqual(TEXT("E13: events only for actual changes"), Events->Count, 4);
    FDoorWorld Four;
    Four.Doors->Variants.AddDefaulted(2);
    Four.Doors->DispatchBeginPlay();
    Four.Doors->HandleKey(EKeys::C);
    TestEqual(TEXT("C selects existing index 2"), Four.Doors->GetActiveVariant(), 2);
    Four.Doors->HandleKey(EKeys::V);
    TestEqual(TEXT("V selects existing index 3"), Four.Doors->GetActiveVariant(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachDoorEmptyWidgetTest, "ReachStudy.S4.E15_MissingPanelConnection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachDoorEmptyWidgetTest::RunTest(const FString& Parameters)
{
    FDoorWorld F;
    UReachDoorPanelWidget* Widget = CreateWidget<UReachDoorPanelWidget>(F.World);
    if (!TestNotNull(TEXT("Widget created"), Widget)) { return false; }
    const TSharedRef<SWidget> Slate = Widget->TakeWidget();
    UHorizontalBox* Row = Cast<UHorizontalBox>(Widget->GetRootWidget());
    if (!TestNotNull(TEXT("Empty row created"), Row)) { return false; }
    TestEqual(TEXT("E15: only message child"), Row->GetChildrenCount(), 1);
    UTextBlock* Message = Cast<UTextBlock>(Row->GetChildAt(0));
    if (!TestNotNull(TEXT("E15: no button, text only"), Message)) { return false; }
    TestEqual(TEXT("E15: missing connection text"), Message->GetText().ToString(), FString(TEXT("No door variants")));
    AddExpectedMessage(TEXT("No door variants"), EAutomationExpectedMessageFlags::Contains, 1);
    AReachDoorPanel* Panel = F.World->SpawnActor<AReachDoorPanel>();
    Panel->DispatchBeginPlay();
    UIsdkInteractableWidgetComponent* Component = Panel->FindComponentByClass<UIsdkInteractableWidgetComponent>();
    if (!TestNotNull(TEXT("Panel has ISDK widget"), Component)) { return false; }
    TestTrue(TEXT("Panel enables poke"), Component->bCreatePokeInteractable);
    TestFalse(TEXT("Panel prohibits widget ray"), Component->bCreateRayInteractable);
    TestTrue(TEXT("Panel uses native widget"), Component->WidgetClass == UReachDoorPanelWidget::StaticClass());
    F.Doors->InitialVariant = 1;
    AReachDoorPanel* Connected = F.World->SpawnActor<AReachDoorPanel>();
    Connected->DoorVariants = F.Doors;
    Connected->DispatchBeginPlay();
    UReachDoorPanelWidget* PanelWidget = Cast<UReachDoorPanelWidget>(Connected->InteractableWidget->GetWidget()->GetUserWidgetObject());
    if (!TestNotNull(TEXT("Panel forwards manager to widget"), PanelWidget)) { return false; }
    const TSharedRef<SWidget> PanelSlate = PanelWidget->TakeWidget();
    UHorizontalBox* PanelRow = CastChecked<UHorizontalBox>(PanelWidget->GetRootWidget());
    TestEqual(TEXT("Connected panel creates variant buttons"), PanelRow->GetChildrenCount(), 2);
    TestTrue(TEXT("Initial color works before manager BeginPlay"), CastChecked<UButton>(PanelRow->GetChildAt(1))->GetBackgroundColor() == PanelWidget->ActiveColor);
    F.Doors->DispatchBeginPlay();
    TestTrue(TEXT("Nonzero initial variant applied without switch"), F.A->IsHidden() && !F.B->IsHidden());
    CastChecked<UButton>(PanelRow->GetChildAt(0))->OnClicked.Broadcast();
    TestEqual(TEXT("Connected panel selects manager"), F.Doors->GetActiveVariant(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachDoorPokeTest, "ReachStudy.S4.E16_HandPokeOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachDoorPokeTest::RunTest(const FString& Parameters)
{
    // Constructor configuration without starting XR in the automation world (E16).
    const AReachHandPawn* Pawn = GetDefault<AReachHandPawn>();
    TArray<UIsdkHandRigComponent*> Rigs;
    Pawn->GetComponents<UIsdkHandRigComponent>(Rigs);
    TestEqual(TEXT("E16: both hand rigs"), Rigs.Num(), 2);
    for (UIsdkHandRigComponent* Rig : Rigs)
    {
        TestTrue(TEXT("E16: poke auto-activates"), Rig->GetPokeInteraction()->bAutoActivate);
        TestFalse(TEXT("E16: ray stays disabled"), Rig->GetRayInteraction()->bAutoActivate);
        TestFalse(TEXT("E16: grab stays disabled"), Rig->GetGrabInteraction()->bAutoActivate);
        TestNull(TEXT("E16: no ray visual"), Rig->GetRayInteraction()->RayVisualClass.Get());
        const FArrayProperty* Property = FindFProperty<FArrayProperty>(Rig->GetClass(), TEXT("RigModifiersToSpawn"));
        if (!TestNotNull(TEXT("E16: modifier list property"), Property)) { return false; }
        const FClassProperty* Inner = CastFieldChecked<FClassProperty>(Property->Inner);
        FScriptArrayHelper Modifiers(Property, Property->ContainerPtrToValuePtr<void>(Rig));
        bool bHasLimiter = false;
        for (int32 Index = 0; Index < Modifiers.Num(); ++Index)
        {
            bHasLimiter |= Inner->GetObjectPropertyValue(Modifiers.GetRawPtr(Index)) == UIsdkPokeLimiterRigModifier::StaticClass();
        }
        TestTrue(TEXT("E16: limiter modifier registered"), bHasLimiter);
        for (EIsdkGrabDetectorType Type : {EIsdkGrabDetectorType::HandGrab, EIsdkGrabDetectorType::DistanceGrab, EIsdkGrabDetectorType::RayGrab})
        {
            TestFalse(TEXT("E16: grab detection blocked"), Rig->GetGrabber()->IsGrabDetectionTypeAllowed(Type));
        }
    }
    return true;
}
#endif

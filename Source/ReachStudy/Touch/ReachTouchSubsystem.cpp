#include "Touch/ReachTouchSubsystem.h"

#include "Components/MeshComponent.h"
#include "Door/ReachDoorVariants.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Hand/ReachHandPawn.h"
#include "IsdkContentAssetPaths.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Touch/ReachTouchComponent.h"
#include "Touch/ReachTouchSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogReachTouchSubsystem, Log, All);

bool UReachTouchSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UReachTouchSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UReachTouchSubsystem, STATGROUP_Tickables);
}

void UReachTouchSubsystem::RegisterTarget(UReachTouchComponent* Component, UMeshComponent* Mesh)
{
    for (const auto& Entry : Targets)
    {
        if (UReachTouchComponent* Other = Entry.Get(); Other && Other->TargetMesh == Mesh)
        {
            UE_LOG(LogReachTouchSubsystem, Warning, TEXT("Mesh already has a ReachTouchComponent: %s"), *Mesh->GetPathName());
            return;
        }
    }
    Component->TargetMesh = Mesh;
    Targets.Add(Component);
}

void UReachTouchSubsystem::UnregisterTarget(UReachTouchComponent* Component)
{
    // Rejected duplicates must not clear the accepted component's overlay (S3 D1).
    ReachContact::ApplyOverlay(Component->TargetMesh.Get(), nullptr);
    Component->TargetMesh.Reset();
    Component->State = {};
    ++Component->EventRevision;
    Targets.Remove(Component);
}

const AReachTouchSettings* UReachTouchSubsystem::ResolveSettings()
{
    if (!bResolvedSettings)
    {
        bResolvedSettings = true;
        int32 Count = 0;
        for (TActorIterator<AReachTouchSettings> It(GetWorld()); It; ++It)
        {
            if (Count++ == 0) { Settings = *It; }
        }
        if (Count == 0) { UE_LOG(LogReachTouchSubsystem, Warning, TEXT("No ReachTouchSettings; using defaults")); }
        else if (Count > 1) { UE_LOG(LogReachTouchSubsystem, Warning, TEXT("Multiple ReachTouchSettings; using first")); }
        if (!Cast<AReachHandPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
        {
            UE_LOG(LogReachTouchSubsystem, Warning, TEXT("Pawn is not ReachHandPawn (hands hidden, no poke)"));
        }
    }
    return Settings.IsValid() ? Settings.Get() : GetDefault<AReachTouchSettings>();
}

void UReachTouchSubsystem::UpdateDoors(const AReachTouchSettings& Config)
{
    AReachDoorVariants* Selected = Config.DoorVariants;
    if (!IsValid(Selected))
    {
        Selected = nullptr;
        for (TActorIterator<AReachDoorVariants> It(GetWorld()); It; ++It) { Selected = *It; break; }
    }
    if (Doors.Get() == Selected) { return; }
    if (Doors.IsValid()) { Doors->OnVariantChanged.RemoveDynamic(this, &UReachTouchSubsystem::HandleVariantChanged); }
    Doors = Selected;
    if (Doors.IsValid()) { Doors->OnVariantChanged.AddUniqueDynamic(this, &UReachTouchSubsystem::HandleVariantChanged); }
}

void UReachTouchSubsystem::Tick(float DeltaSeconds)
{
    if (Targets.IsEmpty()) { return; }
    const AReachTouchSettings& Config = *ResolveSettings();
    const FReachHandSample Left = ReachContact::ReadHand(this, EControllerHand::Left, Config.ContactPart);
    const FReachHandSample Right = ReachContact::ReadHand(this, EControllerHand::Right, Config.ContactPart);
    ProcessSamples(Left, Right, DeltaSeconds, Config);
}

void UReachTouchSubsystem::ProcessFrame(const FXRHandTrackingState& Left, const FXRHandTrackingState& Right, float DeltaSeconds)
{
    if (Targets.IsEmpty()) { return; }
    const AReachTouchSettings& Config = *ResolveSettings();
    ProcessSamples(ReachContact::ReadSample(Left, Config.ContactPart), ReachContact::ReadSample(Right, Config.ContactPart), DeltaSeconds, Config);
}

void UReachTouchSubsystem::ProcessSamples(const FReachHandSample& Left, const FReachHandSample& Right,
    float DeltaSeconds, const AReachTouchSettings& Config)
{
    UpdateDoors(Config);
    if (!DefaultMaterial)
    {
        if (UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, IsdkContentAssetPaths::Materials::GrabbableDefault))
        {
            DefaultMaterial = UMaterialInstanceDynamic::Create(Parent, this);
        }
    }
    if (DefaultMaterial) { DefaultMaterial->SetVectorParameterValue(TEXT("ColorParam"), Config.TouchColor); }
    TArray<FTouchChange> Changes;
    const FReachHandSample* Hands[2] = {&Left, &Right};
    for (const auto& Entry : Targets)
    {
        UReachTouchComponent* Component = Entry.Get();
        if (!Component || !Component->TargetMesh.IsValid()) { continue; }
        UMeshComponent* Mesh = Component->TargetMesh.Get();
        const bool bWasTouching = Component->IsTouching();
        if (Doors.IsValid() && !Doors->IsActorActive(Component->GetOwner()))
        {
            Component->State = {};
        }
        else
        {
            FReachHandContact Contacts[2];
            for (int32 Index = 0; Index < 2; ++Index)
            {
                Contacts[Index].bTracked = Hands[Index]->bTracked;
                if (!ReachContact::IsWithinContactBounds(Mesh, *Hands[Index], Config.ContactDistanceMm, Config.ReleaseMarginMm)) { continue; }
                const FReachContactQueryResult Query = ReachContact::QueryComponent(Mesh, *Hands[Index],
                    Config.ContactDistanceMm, Config.ReleaseMarginMm, Config.bUseJointRadius);
                Contacts[Index].Condition = Config.ContactMethod == EReachContactMethod::SimpleDistance ? Query.Simple : Query.Complex;
            }
            Component->State.Step(Contacts[0], Contacts[1], DeltaSeconds, Config.DwellSeconds, Config.TrackingGraceSeconds);
        }
        const bool bTouching = Component->IsTouching();
        UMaterialInterface* Material = Component->TouchMaterial ? Component->TouchMaterial.Get() : DefaultMaterial.Get();
        ReachContact::ApplyOverlay(Mesh, bTouching ? Material : nullptr);
        if (bTouching != bWasTouching) { Changes.Add({Component, ++Component->EventRevision, bTouching}); }
    }
    DispatchChanges(Changes);
}

void UReachTouchSubsystem::DispatchChanges(const TArray<FTouchChange>& Changes)
{
    // Handlers may remove targets or switch doors (S3 D6/D8).
    for (const FTouchChange& Change : Changes)
    {
        UReachTouchComponent* Component = Change.Component.Get();
        if (!Component || !Targets.Contains(Component) || Component->EventRevision != Change.Revision) { continue; }
        if (Change.bTouching) { Component->OnTouchBegin.Broadcast(Component); }
        else { Component->OnTouchEnd.Broadcast(Component); }
    }
}

void UReachTouchSubsystem::HandleVariantChanged(int32 Index)
{
    TArray<FTouchChange> Changes;
    for (const auto& Entry : Targets)
    {
        UReachTouchComponent* Component = Entry.Get();
        if (!Component) { continue; }
        const bool bWasTouching = Component->IsTouching();
        Component->State = {};
        ++Component->EventRevision;
        ReachContact::ApplyOverlay(Component->TargetMesh.Get(), nullptr);
        if (bWasTouching) { Changes.Add({Component, Component->EventRevision, false}); }
    }
    DispatchChanges(Changes);
}

void UReachTouchSubsystem::Deinitialize()
{
    if (Doors.IsValid()) { Doors->OnVariantChanged.RemoveDynamic(this, &UReachTouchSubsystem::HandleVariantChanged); }
    const auto Snapshot = Targets;
    for (const auto& Entry : Snapshot)
    {
        if (UReachTouchComponent* Component = Entry.Get()) { UnregisterTarget(Component); }
    }
    Targets.Reset();
    DefaultMaterial = nullptr;
    Super::Deinitialize();
}

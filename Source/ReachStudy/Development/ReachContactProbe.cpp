#include "Development/ReachContactProbe.h"

#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Door/ReachDoorVariants.h"
#include "GameFramework/PlayerController.h"
#include "Hand/ReachHandPawn.h"
#include "HAL/PlatformTime.h"
#include "IsdkContentAssetPaths.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

AReachContactProbe::AReachContactProbe()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AReachContactProbe::BeginPlay()
{
    Super::BeginPlay();
    EnsureTouchMaterial();
    if (!IsValid(DoorVariants))
    {
        for (TActorIterator<AReachDoorVariants> It(GetWorld()); It; ++It)
        {
            DoorVariants = *It;
            break;
        }
    }
    if (IsValid(DoorVariants))
    {
        DoorVariants->OnVariantChanged.AddUniqueDynamic(this, &AReachContactProbe::HandleVariantChanged);
    }
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        EnableInput(PC);
        for (const FKey& Key : {EKeys::M, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Equals,
            EKeys::Hyphen, EKeys::RightBracket, EKeys::LeftBracket, EKeys::Period,
            EKeys::Comma, EKeys::Zero, EKeys::Nine, EKeys::R})
        {
            InputComponent->BindKey(Key, IE_Pressed, this, &AReachContactProbe::HandleKey);
        }
    }
}

void AReachContactProbe::EnsureTouchMaterial()
{
    if (TouchMID) { return; }
    UMaterialInterface* Parent = TouchMaterial;
    if (!Parent)
    {
        Parent = LoadObject<UMaterialInterface>(nullptr, IsdkContentAssetPaths::Materials::GrabbableDefault);
    }
    if (Parent)
    {
        TouchMID = UMaterialInstanceDynamic::Create(Parent, this);
        TouchMID->SetVectorParameterValue(TEXT("ColorParam"), TouchColor);
    }
}

void AReachContactProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const AReachHandPawn* Pawn = Cast<AReachHandPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
    const FReachHandSample Left = Pawn ? Pawn->ReadHand(EControllerHand::Left, ContactPart) : FReachHandSample{};
    const FReachHandSample Right = Pawn ? Pawn->ReadHand(EControllerHand::Right, ContactPart) : FReachHandSample{};
    ProcessFrame(Left, Right, DeltaSeconds);
}

void AReachContactProbe::ProcessFrame(const FReachHandSample& Left, const FReachHandSample& Right, float DeltaSeconds)
{
    EnsureTouchMaterial();
    Results.Reset();
    for (auto It = States.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid() || !Targets.Contains(It.Key().Get()))
        {
            ReachContact::ApplyOverlay(It.Key().Get(), nullptr);
            It.RemoveCurrent();
        }
    }
    for (AActor* Target : Targets)
    {
        if (!IsValid(Target)) { continue; }
        const double Start = FPlatformTime::Seconds();
        FReachProbeResult Result;
        Result.Target = Target;
        if (IsValid(DoorVariants) && !DoorVariants->IsActorActive(Target))
        {
            States.Remove(Target);
            ReachContact::ApplyOverlay(Target, nullptr);
            Result.bInactive = true;
            Results.Add(Result);
            continue;
        }
        Result.Hands[0] = ReachContact::Query(Target, Left, ContactDistanceMm, ReleaseMarginMm, bUseJointRadius);
        Result.Hands[1] = ReachContact::Query(Target, Right, ContactDistanceMm, ReleaseMarginMm, bUseJointRadius);
        const bool bSimple = ContactMethod == EReachContactMethod::SimpleDistance;
        FReachContactState& State = States.FindOrAdd(Target);
        const bool bChanged = State.Step(
            {Left.bTracked, bSimple ? Result.Hands[0].Simple : Result.Hands[0].Complex},
            {Right.bTracked, bSimple ? Result.Hands[1].Simple : Result.Hands[1].Complex},
            DeltaSeconds, DwellSeconds, TrackingGraceSeconds);
        if (bChanged)
        {
            ReachContact::ApplyOverlay(Target, State.Phase == EReachContactPhase::Touching ? TouchMID.Get() : nullptr);
        }
        Result.Phase = State.Phase;
        Result.CostMicroseconds = (FPlatformTime::Seconds() - Start) * 1.e6;
        Results.Add(Result);
    }
}

void AReachContactProbe::HandleKey(FKey Key)
{
    if (Key == EKeys::M)
    {
        ContactMethod = ContactMethod == EReachContactMethod::SimpleDistance
            ? EReachContactMethod::ComplexOverlap : EReachContactMethod::SimpleDistance;
    }
    else if (Key == EKeys::One) { ContactPart = EReachContactPart::IndexTip; }
    else if (Key == EKeys::Two) { ContactPart = EReachContactPart::Fingertips; }
    else if (Key == EKeys::Three) { ContactPart = EReachContactPart::WholeHand; }
    else if (Key == EKeys::Equals) { ContactDistanceMm = FMath::Clamp(ContactDistanceMm + 1.0f, 0.0f, 50.0f); }
    else if (Key == EKeys::Hyphen) { ContactDistanceMm = FMath::Clamp(ContactDistanceMm - 1.0f, 0.0f, 50.0f); }
    else if (Key == EKeys::RightBracket) { DwellSeconds = FMath::Clamp(DwellSeconds + 0.05f, 0.0f, 2.0f); }
    else if (Key == EKeys::LeftBracket) { DwellSeconds = FMath::Clamp(DwellSeconds - 0.05f, 0.0f, 2.0f); }
    else if (Key == EKeys::Period) { ReleaseMarginMm = FMath::Clamp(ReleaseMarginMm + 1.0f, 0.0f, 20.0f); }
    else if (Key == EKeys::Comma) { ReleaseMarginMm = FMath::Clamp(ReleaseMarginMm - 1.0f, 0.0f, 20.0f); }
    else if (Key == EKeys::Zero) { TrackingGraceSeconds = FMath::Clamp(TrackingGraceSeconds + 0.05f, 0.0f, 1.0f); }
    else if (Key == EKeys::Nine) { TrackingGraceSeconds = FMath::Clamp(TrackingGraceSeconds - 0.05f, 0.0f, 1.0f); }
    else if (Key == EKeys::R) { bUseJointRadius = !bUseJointRadius; }
    else { return; }
    // Avoid carrying old conditions across changed measurement settings (S2 D4-D7).
    ResetContacts();
}

void AReachContactProbe::HandleVariantChanged(int32 Index)
{
    ResetContacts();
}

void AReachContactProbe::ResetContacts()
{
    for (const auto& Entry : States) { ReachContact::ApplyOverlay(Entry.Key.Get(), nullptr); }
    States.Reset();
    Results.Reset();
}

void AReachContactProbe::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(DoorVariants))
    {
        DoorVariants->OnVariantChanged.RemoveDynamic(this, &AReachContactProbe::HandleVariantChanged);
    }
    ResetContacts();
    Super::EndPlay(EndPlayReason);
}

#include "Door/ReachDoorVariants.h"

#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogReachDoorVariants, Log, All);

AReachDoorVariants::AReachDoorVariants()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AReachDoorVariants::BeginPlay()
{
    Super::BeginPlay();
    for (int32 Index = 0; Index < Variants.Num(); ++Index)
    {
        for (AActor* Root : Variants[Index].Actors)
        {
            if (!IsValid(Root)) { continue; }
            TArray<AActor*> Actors = {Root};
            if (bIncludeAttachedActors) { Root->GetAttachedActors(Actors, false, true); }
            for (AActor* Actor : Actors)
            {
                if (!IsValid(Actor)) { continue; }
                FActorState* State = OriginalStates.Find(Actor);
                if (!State)
                {
                    FActorState Original;
                    Original.bHidden = Actor->IsHidden();
                    Original.bCollision = Actor->GetActorEnableCollision();
                    State = &OriginalStates.Add(Actor, Original);
                }
                State->VariantIndices.AddUnique(Index);
            }
        }
    }
    if (Variants.IsValidIndex(InitialVariant))
    {
        ActiveVariant = InitialVariant;
        ApplySelection();
    }
    else
    {
        UE_LOG(LogReachDoorVariants, Warning, TEXT("Invalid door variant index: %d"), InitialVariant);
    }
    bInitialized = true;
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        EnableInput(PC);
        for (const FKey& Key : {EKeys::Z, EKeys::X, EKeys::C, EKeys::V})
        {
            InputComponent->BindKey(Key, IE_Pressed, this, &AReachDoorVariants::HandleKey);
        }
    }
}

void AReachDoorVariants::SelectVariant(int32 Index)
{
    if (!Variants.IsValidIndex(Index))
    {
        UE_LOG(LogReachDoorVariants, Warning, TEXT("Invalid door variant index: %d"), Index);
        return;
    }
    if (!bInitialized || ActiveVariant == Index) { return; }
    ActiveVariant = Index;
    ApplySelection();
    OnVariantChanged.Broadcast(Index);
}

bool AReachDoorVariants::IsActorActive(AActor* Actor) const
{
    const FActorState* State = OriginalStates.Find(Actor);
    // Shared actors keep their original state in every variant (S4 D3).
    return !State || State->VariantIndices.Num() > 1 || State->VariantIndices.Contains(ActiveVariant);
}

void AReachDoorVariants::ApplySelection()
{
    for (const auto& Entry : OriginalStates)
    {
        AActor* Actor = Entry.Key.Get();
        if (!IsValid(Actor)) { continue; }
        const bool bActive = IsActorActive(Actor);
        Actor->SetActorHiddenInGame(bActive ? Entry.Value.bHidden : true);
        Actor->SetActorEnableCollision(bActive ? Entry.Value.bCollision : false);
    }
}

void AReachDoorVariants::HandleKey(FKey Key)
{
    const int32 Index = Key == EKeys::Z ? 0 : Key == EKeys::X ? 1
        : Key == EKeys::C ? 2 : Key == EKeys::V ? 3 : INDEX_NONE;
    if (Variants.IsValidIndex(Index)) { SelectVariant(Index); }
}

void AReachDoorVariants::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bInitialized = false;
    for (const auto& Entry : OriginalStates)
    {
        if (AActor* Actor = Entry.Key.Get())
        {
            Actor->SetActorHiddenInGame(Entry.Value.bHidden);
            Actor->SetActorEnableCollision(Entry.Value.bCollision);
        }
    }
    OriginalStates.Reset();
    Super::EndPlay(EndPlayReason);
}

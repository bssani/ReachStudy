#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ReachDoorVariants.generated.h"

USTRUCT(BlueprintType)
struct FReachDoorVariant
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach")
    FText Name;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach")
    TArray<TObjectPtr<AActor>> Actors;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReachVariantChanged, int32, Index);

UCLASS()
class REACHSTUDY_API AReachDoorVariants : public AActor
{
    GENERATED_BODY()
public:
    AReachDoorVariants();
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach")
    TArray<FReachDoorVariant> Variants;

    UPROPERTY(EditAnywhere, Category = "Reach")
    bool bIncludeAttachedActors = true;

    UPROPERTY(EditAnywhere, Category = "Reach")
    int32 InitialVariant = 0;

    UPROPERTY(EditAnywhere, Category = "Reach", meta = (ClampMin = "0", Units = "s"))
    double SwitchCooldownSeconds = 0.5;

    UPROPERTY(BlueprintAssignable, Category = "Reach")
    FReachVariantChanged OnVariantChanged;

    UFUNCTION(BlueprintCallable, Category = "Reach")
    void SelectVariant(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Reach")
    void SelectNext();

    UFUNCTION(BlueprintCallable, Category = "Reach")
    void SelectPrevious();

    UFUNCTION(BlueprintPure, Category = "Reach")
    int32 GetActiveVariant() const
    {
        // Panels may construct before this actor begins play (S4 D3/D6).
        return bInitialized ? ActiveVariant : (Variants.IsEmpty() ? INDEX_NONE : (Variants.IsValidIndex(InitialVariant) ? InitialVariant : 0));
    }

    UFUNCTION(BlueprintPure, Category = "Reach")
    bool IsActorActive(AActor* Actor) const;

    void HandleKey(FKey Key);

protected:
    virtual void BeginPlay() override;
    virtual double ReadSwitchTimeSeconds() const;

private:
    struct FActorState
    {
        bool bHidden = false;
        bool bCollision = true;
        TArray<int32> VariantIndices;
    };

    void ApplySelection();
    void SelectRelative(int32 Direction);
    TMap<TWeakObjectPtr<AActor>, FActorState> OriginalStates;
    int32 ActiveVariant = INDEX_NONE;
    bool bInitialized = false;
    double LastSwitchTimeSeconds = -TNumericLimits<double>::Max();
};

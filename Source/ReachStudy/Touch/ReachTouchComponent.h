#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Contact/ReachContact.h"
#include "ReachTouchComponent.generated.h"

class UMaterialInterface;
class UMeshComponent;
class UReachTouchComponent;
class UReachTouchSubsystem;

UENUM(BlueprintType)
enum class EReachTouchPhase : uint8
{
    Idle,
    Dwelling,
    Touching
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReachTouchEvent, UReachTouchComponent*, Component);

UCLASS(ClassGroup = (Reach), meta = (BlueprintSpawnableComponent))
class REACHSTUDY_API UReachTouchComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach")
    TObjectPtr<UMaterialInterface> TouchMaterial;

    UPROPERTY(BlueprintAssignable, Category = "Reach")
    FReachTouchEvent OnTouchBegin;

    UPROPERTY(BlueprintAssignable, Category = "Reach")
    FReachTouchEvent OnTouchEnd;

    UFUNCTION(BlueprintPure, Category = "Reach")
    EReachTouchPhase GetPhase() const;

    UFUNCTION(BlueprintPure, Category = "Reach")
    bool IsTouching() const { return State.Phase == EReachContactPhase::Touching; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class UReachTouchSubsystem;
    TWeakObjectPtr<UMeshComponent> TargetMesh;
    FReachContactState State;
    uint64 EventRevision = 0;
};

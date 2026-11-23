#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Contact/ReachContactQuery.h"
#include "InputCoreTypes.h"
#include "ReachContactProbe.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class AReachDoorVariants;

struct FReachProbeResult
{
    TWeakObjectPtr<AActor> Target;
    FReachContactQueryResult Hands[2];
    EReachContactPhase Phase = EReachContactPhase::Idle;
    double CostMicroseconds = 0.0;
    bool bInactive = false;
};

UCLASS()
class REACHSTUDY_API AReachContactProbe : public AActor
{
    GENERATED_BODY()
public:
    AReachContactProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, Category = "Reach")
    TArray<TObjectPtr<AActor>> Targets;
    UPROPERTY(EditAnywhere, Category = "Reach")
    TObjectPtr<AReachDoorVariants> DoorVariants;
    UPROPERTY(EditAnywhere, Category = "Reach")
    EReachContactPart ContactPart = EReachContactPart::Fingertips;
    UPROPERTY(EditAnywhere, Category = "Reach")
    EReachContactMethod ContactMethod = EReachContactMethod::ComplexOverlap;
    UPROPERTY(EditAnywhere, Category = "Reach", meta = (ClampMin = "0", ClampMax = "50", Units = "mm"))
    float ContactDistanceMm = 10.0f;
    UPROPERTY(EditAnywhere, Category = "Reach", meta = (ClampMin = "0", ClampMax = "20", Units = "mm"))
    float ReleaseMarginMm = 5.0f;
    UPROPERTY(EditAnywhere, Category = "Reach", meta = (ClampMin = "0", ClampMax = "2", Units = "s"))
    float DwellSeconds = 0.15f;
    UPROPERTY(EditAnywhere, Category = "Reach", meta = (ClampMin = "0", ClampMax = "1", Units = "s"))
    float TrackingGraceSeconds = 0.1f;
    UPROPERTY(EditAnywhere, Category = "Reach")
    bool bUseJointRadius = false;
    UPROPERTY(EditAnywhere, Category = "Reach")
    TObjectPtr<UMaterialInterface> TouchMaterial;
    UPROPERTY(EditAnywhere, Category = "Reach")
    FLinearColor TouchColor = FLinearColor::Green;

    void HandleKey(FKey Key);
    void ProcessFrame(const FReachHandSample& Left, const FReachHandSample& Right, float DeltaSeconds);
    const TArray<FReachProbeResult>& GetResults() const { return Results; }

protected:
    virtual void BeginPlay() override;

private:
    void ResetContacts();
    void EnsureTouchMaterial();
    UFUNCTION()
    void HandleVariantChanged(int32 Index);

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> TouchMID;
    TMap<TWeakObjectPtr<AActor>, FReachContactState> States;
    TArray<FReachProbeResult> Results;
};

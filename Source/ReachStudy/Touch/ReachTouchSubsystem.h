#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Contact/ReachContactQuery.h"
#include "ReachTouchSubsystem.generated.h"

class AReachDoorVariants;
class AReachTouchSettings;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UReachTouchComponent;

UCLASS()
class REACHSTUDY_API UReachTouchSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;
    virtual void Deinitialize() override;

    void RegisterTarget(UReachTouchComponent* Component, UMeshComponent* Mesh);
    void UnregisterTarget(UReachTouchComponent* Component);
    void ProcessFrame(const FXRHandTrackingState& Left, const FXRHandTrackingState& Right, float DeltaSeconds);

protected:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
    struct FTouchChange
    {
        TWeakObjectPtr<UReachTouchComponent> Component;
        uint64 Revision = 0;
        bool bTouching = false;
    };

    const AReachTouchSettings* ResolveSettings();
    void UpdateDoors(const AReachTouchSettings& Config);
    void ProcessSamples(const FReachHandSample& Left, const FReachHandSample& Right,
        float DeltaSeconds, const AReachTouchSettings& Config);
    void DispatchChanges(const TArray<FTouchChange>& Changes);

    UFUNCTION()
    void HandleVariantChanged(int32 Index);

    TArray<TWeakObjectPtr<UReachTouchComponent>> Targets;
    TWeakObjectPtr<AReachTouchSettings> Settings;
    TWeakObjectPtr<AReachDoorVariants> Doors;
    bool bResolvedSettings = false;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DefaultMaterial;
};

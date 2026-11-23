#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Door/ReachDoorVariants.h"
#include "ReachDoorTestObserver.generated.h"

UCLASS()
class UReachDoorTestObserver : public UObject
{
    GENERATED_BODY()
public:
    int32 Count = 0;
    int32 LastIndex = INDEX_NONE;

    UFUNCTION()
    void Record(int32 Index) { ++Count; LastIndex = Index; }
};

UCLASS()
class AReachDoorTestVariants : public AReachDoorVariants
{
    GENERATED_BODY()
public:
    double NowSeconds = 100.0;

protected:
    virtual double ReadSwitchTimeSeconds() const override { return NowSeconds; }
};

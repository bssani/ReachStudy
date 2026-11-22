#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
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

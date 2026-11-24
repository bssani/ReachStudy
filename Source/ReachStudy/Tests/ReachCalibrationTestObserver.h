#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"
#include "ReachCalibrationTestObserver.generated.h"

UCLASS()
class AReachCalibrationTestObserver : public AActor
{
    GENERATED_BODY()
public:
    float BeginPlayWorldToMeters = 0.0f;

protected:
    virtual void BeginPlay() override
    {
        Super::BeginPlay();
        BeginPlayWorldToMeters = GetWorld()->GetWorldSettings()->WorldToMeters;
    }
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ReachGameMode.generated.h"

UCLASS()
class REACHSTUDY_API AReachGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AReachGameMode();
    virtual void StartPlay() override;
};

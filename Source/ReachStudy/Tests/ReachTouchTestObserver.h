#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Touch/ReachTouchComponent.h"
#include "ReachTouchTestObserver.generated.h"

UCLASS()
class UReachTouchTestObserver : public UObject
{
    GENERATED_BODY()
public:
    int32 Begins = 0;
    int32 Ends = 0;
    TWeakObjectPtr<UReachTouchComponent> DestroyOnBegin;
    TWeakObjectPtr<UReachTouchComponent> Other;
    bool bOtherWasTouching = false;

    UFUNCTION()
    void Begin(UReachTouchComponent* Component)
    {
        ++Begins;
        bOtherWasTouching = Other.IsValid() && Other->IsTouching();
        if (DestroyOnBegin.IsValid()) { DestroyOnBegin->DestroyComponent(); }
    }

    UFUNCTION()
    void End(UReachTouchComponent* Component) { ++Ends; }
};

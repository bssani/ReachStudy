#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ReachScreenMessageSubsystem.generated.h"

UCLASS()
class REACHSTUDY_API UReachScreenMessageSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

private:
    bool bPreviousMessagesEnabled = false;
    bool bMessagesDisabled = false;
};

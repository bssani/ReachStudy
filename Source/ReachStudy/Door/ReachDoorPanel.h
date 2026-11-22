#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReachDoorPanel.generated.h"

class AReachDoorVariants;
class UIsdkInteractableWidgetComponent;

UCLASS()
class REACHSTUDY_API AReachDoorPanel : public AActor
{
    GENERATED_BODY()
public:
    AReachDoorPanel();

    UPROPERTY(EditAnywhere, Category = "Reach")
    TObjectPtr<AReachDoorVariants> DoorVariants;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reach")
    TObjectPtr<UIsdkInteractableWidgetComponent> InteractableWidget;

protected:
    virtual void BeginPlay() override;
};

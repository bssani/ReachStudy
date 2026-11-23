#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Contact/ReachContactQuery.h"
#include "ReachTouchSettings.generated.h"

class AReachDoorVariants;

UCLASS()
class REACHSTUDY_API AReachTouchSettings : public AActor
{
    GENERATED_BODY()
public:
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
    FLinearColor TouchColor = FLinearColor::Green;

    UPROPERTY(EditAnywhere, Category = "Reach")
    TObjectPtr<AReachDoorVariants> DoorVariants;
};

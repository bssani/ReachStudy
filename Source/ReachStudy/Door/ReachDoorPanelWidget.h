#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ReachDoorPanelWidget.generated.h"

class AReachDoorVariants;
class UHorizontalBox;

UCLASS()
class UReachDoorVariantButton : public UButton
{
    GENERATED_BODY()
public:
    void Configure(AReachDoorVariants* InDoorVariants, int32 InIndex);

private:
    UFUNCTION()
    void SelectDoor();

    UPROPERTY(Transient)
    TObjectPtr<AReachDoorVariants> DoorVariants;
    int32 Index = INDEX_NONE;
};

UCLASS()
class REACHSTUDY_API UReachDoorPanelWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Reach")
    FLinearColor ActiveColor = FLinearColor(0.1f, 0.6f, 0.25f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Reach")
    FLinearColor IdleColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Reach", meta = (ClampMin = "1"))
    int32 FontSize = 24;

    UPROPERTY(EditAnywhere, Category = "Reach")
    FText NoDoorVariantsText = NSLOCTEXT("ReachDoorPanel", "NoDoorVariants", "No door variants");

    void SetDoorVariants(AReachDoorVariants* InDoorVariants);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void BuildButtons();

    UFUNCTION()
    void RefreshColors(int32 Index);

    UPROPERTY(Transient)
    TObjectPtr<AReachDoorVariants> DoorVariants;
    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> Row;
};

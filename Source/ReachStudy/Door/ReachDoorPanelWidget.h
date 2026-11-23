#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ReachDoorPanelWidget.generated.h"

class AReachDoorVariants;
class UHorizontalBox;
class UTextBlock;

UCLASS()
class REACHSTUDY_API UReachDoorPanelWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Reach")
    FText PreviousButtonText = NSLOCTEXT("ReachDoorPanel", "Previous", "<");

    UPROPERTY(EditAnywhere, Category = "Reach")
    FText NextButtonText = NSLOCTEXT("ReachDoorPanel", "Next", ">");

    UPROPERTY(EditAnywhere, Category = "Reach")
    FLinearColor ButtonColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Reach")
    FLinearColor TextColor = FLinearColor::White;

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
    void RefreshLabel(int32 Index);

    UFUNCTION()
    void SelectPrevious();

    UFUNCTION()
    void SelectNext();

    UPROPERTY(Transient)
    TObjectPtr<AReachDoorVariants> DoorVariants;
    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> Row;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CurrentDoorLabel;
};

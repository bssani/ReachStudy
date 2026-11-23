#include "Door/ReachDoorPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Door/ReachDoorVariants.h"

void UReachDoorPanelWidget::SelectPrevious()
{
    if (IsValid(DoorVariants)) { DoorVariants->SelectPrevious(); }
}

void UReachDoorPanelWidget::SelectNext()
{
    if (IsValid(DoorVariants)) { DoorVariants->SelectNext(); }
}

void UReachDoorPanelWidget::SetDoorVariants(AReachDoorVariants* InDoorVariants)
{
    if (IsValid(DoorVariants))
    {
        DoorVariants->OnVariantChanged.RemoveDynamic(this, &UReachDoorPanelWidget::RefreshLabel);
    }
    DoorVariants = InDoorVariants;
    if (IsValid(DoorVariants))
    {
        DoorVariants->OnVariantChanged.AddUniqueDynamic(this, &UReachDoorPanelWidget::RefreshLabel);
    }
    if (Row) { BuildButtons(); }
}

TSharedRef<SWidget> UReachDoorPanelWidget::RebuildWidget()
{
    if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
    Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    WidgetTree->RootWidget = Row;
    BuildButtons();
    return Super::RebuildWidget();
}

void UReachDoorPanelWidget::BuildButtons()
{
    Row->ClearChildren();
    CurrentDoorLabel = nullptr;
    if (!IsValid(DoorVariants))
    {
        UTextBlock* Message = WidgetTree->ConstructWidget<UTextBlock>();
        Message->SetText(NoDoorVariantsText);
        FSlateFontInfo Font = Message->GetFont();
        Font.Size = FontSize;
        Message->SetFont(Font);
        Row->AddChildToHorizontalBox(Message);
        return;
    }
    const auto AddButton = [this](const FText& Text, bool bNext)
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>();
        Button->SetBackgroundColor(ButtonColor);
        if (bNext) { Button->OnClicked.AddUniqueDynamic(this, &UReachDoorPanelWidget::SelectNext); }
        else { Button->OnClicked.AddUniqueDynamic(this, &UReachDoorPanelWidget::SelectPrevious); }
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(Text);
        Label->SetColorAndOpacity(TextColor);
        FSlateFontInfo Font = Label->GetFont();
        Font.Size = FontSize;
        Label->SetFont(Font);
        Button->AddChild(Label);
        UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button);
        ButtonSlot->SetPadding(FMargin(4.0f));
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
    };
    AddButton(PreviousButtonText, false);
    CurrentDoorLabel = WidgetTree->ConstructWidget<UTextBlock>();
    FSlateFontInfo Font = CurrentDoorLabel->GetFont();
    Font.Size = FontSize;
    CurrentDoorLabel->SetFont(Font);
    CurrentDoorLabel->SetColorAndOpacity(TextColor);
    UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(CurrentDoorLabel);
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LabelSlot->SetHorizontalAlignment(HAlign_Center);
    LabelSlot->SetVerticalAlignment(VAlign_Center);
    AddButton(NextButtonText, true);
    RefreshLabel(DoorVariants->GetActiveVariant());
}

void UReachDoorPanelWidget::RefreshLabel(int32 Index)
{
    if (!CurrentDoorLabel || !IsValid(DoorVariants)) { return; }
    CurrentDoorLabel->SetText(DoorVariants->Variants.IsValidIndex(Index)
        ? FText::Format(NSLOCTEXT("ReachDoorPanel", "CurrentDoor", "{0} ({1}/{2})"),
            DoorVariants->Variants[Index].Name, FText::AsNumber(Index + 1), FText::AsNumber(DoorVariants->Variants.Num()))
        : FText::GetEmpty());
}

void UReachDoorPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (IsValid(DoorVariants))
    {
        DoorVariants->OnVariantChanged.AddUniqueDynamic(this, &UReachDoorPanelWidget::RefreshLabel);
        RefreshLabel(DoorVariants->GetActiveVariant());
    }
}

void UReachDoorPanelWidget::NativeDestruct()
{
    if (IsValid(DoorVariants))
    {
        DoorVariants->OnVariantChanged.RemoveDynamic(this, &UReachDoorPanelWidget::RefreshLabel);
    }
    Super::NativeDestruct();
}

#include "Door/ReachDoorPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Door/ReachDoorVariants.h"

void UReachDoorVariantButton::Configure(AReachDoorVariants* InDoorVariants, int32 InIndex)
{
    DoorVariants = InDoorVariants;
    Index = InIndex;
    OnClicked.AddUniqueDynamic(this, &UReachDoorVariantButton::SelectDoor);
}

void UReachDoorVariantButton::SelectDoor()
{
    if (IsValid(DoorVariants)) { DoorVariants->SelectVariant(Index); }
}

void UReachDoorPanelWidget::SetDoorVariants(AReachDoorVariants* InDoorVariants)
{
    if (IsValid(DoorVariants))
    {
        DoorVariants->OnVariantChanged.RemoveDynamic(this, &UReachDoorPanelWidget::RefreshColors);
    }
    DoorVariants = InDoorVariants;
    if (IsValid(DoorVariants))
    {
        DoorVariants->OnVariantChanged.AddUniqueDynamic(this, &UReachDoorPanelWidget::RefreshColors);
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
    for (int32 Index = 0; Index < DoorVariants->Variants.Num(); ++Index)
    {
        UReachDoorVariantButton* Button = WidgetTree->ConstructWidget<UReachDoorVariantButton>();
        Button->Configure(DoorVariants, Index);
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(DoorVariants->Variants[Index].Name);
        FSlateFontInfo Font = Label->GetFont();
        Font.Size = FontSize;
        Label->SetFont(Font);
        Button->AddChild(Label);
        UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button);
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ButtonSlot->SetPadding(FMargin(4.0f));
    }
    RefreshColors(DoorVariants->GetActiveVariant());
}

void UReachDoorPanelWidget::RefreshColors(int32 Index)
{
    if (!Row) { return; }
    for (int32 Child = 0; Child < Row->GetChildrenCount(); ++Child)
    {
        if (UButton* Button = Cast<UButton>(Row->GetChildAt(Child)))
        {
            Button->SetBackgroundColor(Child == Index ? ActiveColor : IdleColor);
        }
    }
}

void UReachDoorPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (IsValid(DoorVariants))
    {
        DoorVariants->OnVariantChanged.AddUniqueDynamic(this, &UReachDoorPanelWidget::RefreshColors);
        RefreshColors(DoorVariants->GetActiveVariant());
    }
}

void UReachDoorPanelWidget::NativeDestruct()
{
    if (IsValid(DoorVariants))
    {
        DoorVariants->OnVariantChanged.RemoveDynamic(this, &UReachDoorPanelWidget::RefreshColors);
    }
    Super::NativeDestruct();
}

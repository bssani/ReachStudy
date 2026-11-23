#include "Door/ReachDoorPanel.h"

#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Door/ReachDoorPanelWidget.h"
#include "Door/ReachDoorVariants.h"
#include "IsdkInteractableWidgetComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogReachDoorPanel, Log, All);

AReachDoorPanel::AReachDoorPanel()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    InteractableWidget = CreateDefaultSubobject<UIsdkInteractableWidgetComponent>(TEXT("DoorWidget"));
    InteractableWidget->SetupAttachment(RootComponent);
    InteractableWidget->bCreatePokeInteractable = true;
    InteractableWidget->bCreateRayInteractable = false;
    InteractableWidget->WidgetClass = UReachDoorPanelWidget::StaticClass();
    InteractableWidget->DrawSize = FVector2D(420.0, 140.0);
    InteractableWidget->WidgetScale = 0.06f;
}

void AReachDoorPanel::BeginPlay()
{
    Super::BeginPlay();
    if (!IsValid(DoorVariants) || DoorVariants->Variants.IsEmpty())
    {
        UE_LOG(LogReachDoorPanel, Warning, TEXT("No door variants"));
    }
    UReachDoorPanelWidget* Widget = Cast<UReachDoorPanelWidget>(InteractableWidget->GetWidget()->GetUserWidgetObject());
    if (Widget) { Widget->SetDoorVariants(DoorVariants); }
}

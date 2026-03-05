#include "KillfeedWidget.h"
#include "Components/VerticalBox.h"
#include "TimerManager.h"

void UKillfeedWidget::AddKillMessage(FKillfeedData KillData)
{
	if (!MessageContainer || !MessageWidgetClass) return;

	// 1. Array-Shifting Math: If we hit max, delete the top one (oldest) immediately
	if (MessageContainer->GetChildrenCount() >= MaxMessages)
	{
		MessageContainer->RemoveChildAt(0);
		// Note: We don't even need to cancel its timer! If it fires later, 
		// RemoveSpecificMessage will safely see it is already gone and do nothing.
	}

	// 2. Spawn the new row and push the data in
	if (UKillfeedItemWidget* NewMessage = CreateWidget<UKillfeedItemWidget>(this, MessageWidgetClass))
	{
		NewMessage->SetupKillfeedItem(KillData);
		MessageContainer->AddChildToVerticalBox(NewMessage);

		// 3. START TARGETED TIMER
		// We pass a TWeakObjectPtr. If the widget gets force-deleted by the MaxMessages limit 
		// and Garbage Collected early, the timer safely catches the null pointer!
		FTimerHandle NewTimer;
		FTimerDelegate Del = FTimerDelegate::CreateUObject(this, &UKillfeedWidget::RemoveSpecificMessage, TWeakObjectPtr<UKillfeedItemWidget>(NewMessage));

		GetWorld()->GetTimerManager().SetTimer(
			NewTimer,
			Del,
			MessageDisplayTime,
			false
		);
	}
}

void UKillfeedWidget::RemoveSpecificMessage(TWeakObjectPtr<UKillfeedItemWidget> MessageToRemove)
{
	// 1. IsValid() checks if it hasn't been Garbage Collected
	// 2. HasChild checks if it wasn't already force-deleted by the MaxMessages limit
	if (MessageToRemove.IsValid() && MessageContainer && MessageContainer->HasChild(MessageToRemove.Get()))
	{
		MessageContainer->RemoveChild(MessageToRemove.Get());
	}
}
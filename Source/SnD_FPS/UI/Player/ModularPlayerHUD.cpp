#include "ModularPlayerHUD.h"
#include "Components/NamedSlot.h"

void UModularPlayerHUD::SetGameModeWidget(UUserWidget* GameModeWidget, EGameModeWidgetPosition Position)
{
	// 1. Clear out both slots to ensure we don't have overlapping UI from previous rounds
	if (TopCenterScoreSlot) TopCenterScoreSlot->ClearChildren();
	if (BottomLeftScoreSlot) BottomLeftScoreSlot->ClearChildren();

	// 2. If we passed in a valid widget, route it to the correct slot
	if (GameModeWidget)
	{
		switch (Position)
		{
		case EGameModeWidgetPosition::TopCenter:
			if (TopCenterScoreSlot) TopCenterScoreSlot->AddChild(GameModeWidget);
			break;

		case EGameModeWidgetPosition::BottomLeft:
			if (BottomLeftScoreSlot) BottomLeftScoreSlot->AddChild(GameModeWidget);
			break;
		}
	}
}

void UModularPlayerHUD::SetCompassLocation(bool bUnderMinimap)
{
	// Logic to move the compass will go here
}
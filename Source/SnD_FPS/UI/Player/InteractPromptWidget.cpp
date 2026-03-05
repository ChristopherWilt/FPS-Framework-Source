#include "InteractPromptWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "SnD_FPS/Weapon/Weapon.h"

void UInteractPromptWidget::ShowPrompt(AWeapon* WeaponToShow)
{
	if (!WeaponToShow) return;

	// 1. Update the Text
	if (TXT_WeaponName)
	{
		TXT_WeaponName->SetText(WeaponToShow->WeaponName);
	}

	// 2. Update the Icon
	if (IMG_WeaponIcon)
	{
		if (WeaponToShow->WeaponIcon)
		{
			IMG_WeaponIcon->SetBrushFromTexture(WeaponToShow->WeaponIcon);
			IMG_WeaponIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IMG_WeaponIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 3. Make the widget visible on screen
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UInteractPromptWidget::HidePrompt()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
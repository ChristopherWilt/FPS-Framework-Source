// WeaponBuyEntryWidget.cpp

#include "WeaponBuyEntryWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "SnD_FPS/Player/PlayerCharacter.h"
#include "SnD_FPS/Weapon/Weapon.h" // Must include the full definition of AWeapon to access its defaults

void UWeaponBuyEntryWidget::SetWeaponData(TSubclassOf<AWeapon> InWeaponClass, int32 PlayerCurrentMoney)
{
	if (!InWeaponClass) return;

	WeaponClassToBuy = InWeaponClass;

	// Get the Class Default Object (CDO) to read its properties without needing an instance.
	AWeapon* DefaultWeapon = InWeaponClass->GetDefaultObject<AWeapon>();
	if (!DefaultWeapon) return;

	// Update the UI elements with the provided data from the weapon's defaults.
	// Make sure you have set these values in your AWeapon-derived Blueprints.
	if (WeaponNameText)
	{
		WeaponNameText->SetText(DefaultWeapon->WeaponName);
	}
	if (WeaponCostText)
	{
		// This creates a text object from an integer, which is better for localization.
		WeaponCostText->SetText(FText::AsNumber(DefaultWeapon->WeaponCost));
	}
	if (WeaponIconImage && DefaultWeapon->WeaponIcon)
	{
		WeaponIconImage->SetBrushFromTexture(DefaultWeapon->WeaponIcon);
		WeaponIconImage->SetVisibility(ESlateVisibility::Visible);
	}
	else if (WeaponIconImage)
	{
		WeaponIconImage->SetVisibility(ESlateVisibility::Hidden);
	}

	// --- NEW AFFORDABILITY LOGIC ---
	if (BuyButton)
	{
		const bool bCanAfford = PlayerCurrentMoney >= DefaultWeapon->WeaponCost;
		BuyButton->SetIsEnabled(bCanAfford);

		// Optional: Change color for better feedback
		if (!bCanAfford)
		{
			BuyButton->SetColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 1.0f)); // Greyed out
		}
	}
}

void UWeaponBuyEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind our C++ functions to the delegate events of the button.
	// This is how we give the UI functionality from code.
	if (BuyButton)
	{
		BuyButton->OnClicked.AddDynamic(this, &UWeaponBuyEntryWidget::OnBuyButtonClicked);
		BuyButton->OnHovered.AddDynamic(this, &UWeaponBuyEntryWidget::OnBuyButtonHovered);
		BuyButton->OnUnhovered.AddDynamic(this, &UWeaponBuyEntryWidget::OnBuyButtonUnhovered);
	}
}

void UWeaponBuyEntryWidget::OnBuyButtonClicked()
{
	// Get the local player character. Using GetOwningPlayerPawn is safer inside a widget.
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());

	if (PlayerCharacter && WeaponClassToBuy)
	{
		// Tell the player character (which will then tell the server) to purchase this weapon.
		PlayerCharacter->RequestPurchaseWeapon(WeaponClassToBuy);
	}
}

void UWeaponBuyEntryWidget::OnBuyButtonHovered()
{
	// To change the color on hover, we can set the button's background color.
	// A better method is to set the button's style in the Widget Blueprint to a new style that has a different hover image.
	if (BuyButton)
	{
		FLinearColor HoverColor(0.2f, 0.8f, 0.2f, 1.0f); // A bright green color for feedback
		BuyButton->SetBackgroundColor(HoverColor);
	}

	// Broadcast the event and send our weapon class with it
	OnWeaponEntryHovered.Broadcast(WeaponClassToBuy);
}

void UWeaponBuyEntryWidget::OnBuyButtonUnhovered()
{
	// When the mouse leaves, we set the color back to its normal state.
	if (BuyButton)
	{
		FLinearColor NormalColor(1.0f, 1.0f, 1.0f, 1.0f); // Default white
		BuyButton->SetBackgroundColor(NormalColor);
	}
}

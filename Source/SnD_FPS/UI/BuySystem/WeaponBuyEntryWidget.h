// WeaponBuyEntryWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponBuyEntryWidget.generated.h"

// Forward declare the classes we will need
class UButton;
class UImage;
class UTextBlock;
class AWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponHovered, TSubclassOf<AWeapon>, WeaponClass);

/**
 * Represents a single weapon entry in the buy menu UI.
 * This widget is responsible for displaying weapon info and handling click events.
 */
UCLASS()
class SND_FPS_API UWeaponBuyEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Sets the weapon data for this widget to display.
	 * This should be called after the widget is created.
	 * @param InWeaponClass The weapon class this entry represents.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetWeaponData(TSubclassOf<AWeapon> InWeaponClass, int32 PlayerCurrentMoney);

	UPROPERTY(BlueprintAssignable)
	FOnWeaponHovered OnWeaponEntryHovered;

protected:
	// This function is called when the widget is constructed in the game.
	// We use it to bind our C++ functions to the button's events.
	virtual void NativeConstruct() override;

	// --- WIDGET BINDINGS ---
	// These UPROPERTYs link C++ variables to the widgets you create in the UMG editor.
	// The name of the variable MUST match the name of the widget in the UMG Designer.

	// The button that the player clicks to buy the weapon.
	UPROPERTY(meta = (BindWidget))
	UButton* BuyButton;

	// The image that displays the weapon's icon.
	UPROPERTY(meta = (BindWidget))
	UImage* WeaponIconImage;

	// The text block for the weapon's name.
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponNameText;

	// The text block for the weapon's cost.
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponCostText;

private:
	// --- STATE & DATA ---

	/** The actual weapon class that will be purchased if this entry is clicked. UPROPERTY is needed to prevent it from being garbage collected. */
	UPROPERTY()
	TSubclassOf<AWeapon> WeaponClassToBuy;

	// --- HOVER & CLICK HANDLERS ---

	/** Called when the player clicks the BuyButton. */
	UFUNCTION()
	void OnBuyButtonClicked();

	/** Called when the player's mouse hovers over the BuyButton. */
	UFUNCTION()
	void OnBuyButtonHovered();

	/** Called when the player's mouse leaves the BuyButton. */
	UFUNCTION()
	void OnBuyButtonUnhovered();
};

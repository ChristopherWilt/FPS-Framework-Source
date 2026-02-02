// WeaponBuyMenuWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponBuyMenuWidget.generated.h"

class UVerticalBox;
class UWeaponBuyEntryWidget;
class UPlayerInfoEntryWidget;

/**
 * The main widget for the weapon buy menu screen.
 * This widget will be populated with UWeaponBuyEntryWidgets.
 */
UCLASS()
class SND_FPS_API UWeaponBuyMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// This function is called when the widget is constructed.
	// It's the perfect place to populate our buy menu.
	virtual void NativeConstruct() override;

	// --- WIDGET BINDINGS ---

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* Pistols_Column;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* SMGs_Column;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* Shotguns_Column;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* ARs_Column;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* Snipers_Column;

	// Add this for the specs panel too
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* WeaponSpecs_Panel;

	// ... you will also need BindWidget properties for all the text/image blocks
	// in the left and right panels you want to update.

	// --- DATA & CONFIGURATION ---

	// This stores a reference to the WeaponBuyEntryWidget Blueprint.
	// We need to set this in the editor so we know which widget to create.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UWeaponBuyEntryWidget> WeaponEntryWidgetClass;

	// We will need a way to define which weapons are for sale.
	// A Data Table is the best way to do this, but for simplicity,
	// we'll start with an array of weapon classes.
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TArray<TSubclassOf<class AWeapon>> WeaponsForSale;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PlayerInfoEntryWidgetClass;

	// This is the Vertical Box you created in the UMG editor for the left panel.
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* PlayerInfo_Panel;

	// --- HANDLERS ---

	UFUNCTION()
	void OnWeaponHovered(TSubclassOf<AWeapon> WeaponClass);
};

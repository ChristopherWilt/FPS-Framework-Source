#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SnD_FPS/Weapon/WeaponEnums.h"
#include "WeaponInventoryWidget.generated.h"

class UTexture2D;

UCLASS()
class SND_FPS_API UWeaponInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 1. Ammo
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory UI")
	void UpdateAmmo(int32 CurrentMag, int32 ReserveAmmo);

	// 2. Fire Modes
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory UI")
	void UpdateFireMode(EFireMode CurrentMode, bool bCanSingle, bool bCanBurst, bool bCanAuto);

	// 3. Weapons (Active + Your Custom Stowed Layout)
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory UI")
	void UpdateWeaponIcons(UTexture2D* ActiveWeapon, UTexture2D* StowedPrimary, UTexture2D* StowedSecondary, UTexture2D* MeleeIcon);

	// 4. Equipment
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory UI")
	void UpdateEquipment(UTexture2D* TacticalIcon, int32 TacticalCount, UTexture2D* LethalIcon, int32 LethalCount);

	// 5. Game Mode Objective (The Bomb)
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory UI")
	void UpdateObjectiveItem(bool bHasObjectiveItem, UTexture2D* ItemIcon);
};
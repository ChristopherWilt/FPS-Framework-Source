#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ModularPlayerHUD.generated.h"

class UNamedSlot;
class UCompassWidget;
class UMinimapWidget;
class UWeaponInventoryWidget;
class UKillfeedWidget;
class UCrosshairWidget;
class UInteractPromptWidget;

UENUM(BlueprintType)
enum class EGameModeWidgetPosition : uint8
{
	TopCenter	UMETA(DisplayName = "Top Center (Valorant/SnD)"),
	BottomLeft	UMETA(DisplayName = "Bottom Left (MW2019/TDM)")
};

UCLASS()
class SND_FPS_API UModularPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- COMPOSITION SLOTS ---

	// Slot for Valorant-style Top Bar
	UPROPERTY(meta = (BindWidget))
	UNamedSlot* TopCenterScoreSlot;

	// Slot for MW2019-style Bottom Left Score
	UPROPERTY(meta = (BindWidget))
	UNamedSlot* BottomLeftScoreSlot;

	// --- STANDARD WIDGETS ---
	UPROPERTY(meta = (BindWidget))
	UMinimapWidget* Minimap;

	UPROPERTY(meta = (BindWidget))
	UCompassWidget* Compass;

	UPROPERTY(meta = (BindWidget))
	UWeaponInventoryWidget* WeaponInventory;

	UPROPERTY(meta = (BindWidget))
	UKillfeedWidget* Killfeed;

	UPROPERTY(meta = (BindWidget))
	UCrosshairWidget* Crosshair;

	UPROPERTY(meta = (BindWidget))
	UInteractPromptWidget* InteractPrompt;

	// --- FUNCTIONALITY ---

	/** * Injects the game mode specific widget into the correct location on the screen.
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD|Modularity")
	void SetGameModeWidget(UUserWidget* GameModeWidget, EGameModeWidgetPosition Position);

	UFUNCTION(BlueprintCallable, Category = "HUD|Settings")
	void SetCompassLocation(bool bUnderMinimap);
};
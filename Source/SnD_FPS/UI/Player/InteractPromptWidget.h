#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractPromptWidget.generated.h"

class UTextBlock;
class UImage;
class AWeapon;

UCLASS()
class SND_FPS_API UInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Pushes the weapon data to the UI and makes it visible */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ShowPrompt(AWeapon* WeaponToShow);

	/** Hides the prompt when looking away */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void HidePrompt();

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_WeaponName;

	UPROPERTY(meta = (BindWidget))
	UImage* IMG_WeaponIcon;
};
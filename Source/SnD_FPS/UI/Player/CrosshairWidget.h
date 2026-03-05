#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDEnums.h"
#include "CrosshairWidget.generated.h"

UCLASS()
class SND_FPS_API UCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Call this when the player changes settings or fires (to expand gap) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crosshair")
	void ApplyCrosshairConfig(FCrosshairConfig NewConfig, float CurrentSpread);
};
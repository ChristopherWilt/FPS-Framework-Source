#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CompassWidget.generated.h"

UCLASS()
class SND_FPS_API UCompassWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** * @param StripYaw Used to slide the widget (-180 to 180)
	 * @param DisplayYaw Used for the center yellow text (0 to 359)
	 * @param LocationName The name of the map area
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Compass")
	void UpdateCompass(float StripYaw, int32 DisplayYaw, const FString& LocationName);
};
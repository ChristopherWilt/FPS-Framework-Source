#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDEnums.h" 
#include "MinimapWidget.generated.h"

UCLASS()
class SND_FPS_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Minimap")
	void SetMinimapShape(EMinimapShape NewShape);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Minimap")
	void SetMinimapZoom(float ZoomLevel);

	// Fires every frame to update the map's position and rotation ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void UpdateMinimap(FVector2D PlayerWorldLocation, float PlayerYaw, float PlayerFOV);

	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void ApplyMinimapSettings(FMinimapConfig Config);

	// --- Spawn the Red Dot ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Minimap")
	void SpawnEnemyPing(AActor* Shooter, FVector PingWorldLocation);
};
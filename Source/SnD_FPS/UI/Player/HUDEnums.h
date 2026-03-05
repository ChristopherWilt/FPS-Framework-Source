#pragma once
#include "CoreMinimal.h"
#include "HUDEnums.generated.h"

UENUM(BlueprintType)
enum class EMinimapShape : uint8
{
	Square UMETA(DisplayName = "Square"),
	Round  UMETA(DisplayName = "Round")
};

USTRUCT(BlueprintType)
struct FCrosshairConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	FLinearColor CrosshairColor = FLinearColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	bool bShowCenterDot = true;

	// --- NEW: Toggle for the 4 directional lines ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	bool bShowOuterLines = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float CenterDotSize = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float LineLength = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float LineThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float BaseOffset = 5.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	bool bIsDynamic = true; 
};

// --- Minimap Settings ---
USTRUCT(BlueprintType)
struct FMinimapConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	EMinimapShape MapShape = EMinimapShape::Square;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float CurrentSize = 275.0f; // Can be adjusted by player

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float MinSize = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float MaxSize = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float ZoomLevel = 1.0f;
};

// --- Enemy Ping Data ---
USTRUCT(BlueprintType)
struct FEnemyPing
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FVector WorldLocation;

	UPROPERTY(BlueprintReadOnly)
	float ExpireTime; // When the red dot disappears
};
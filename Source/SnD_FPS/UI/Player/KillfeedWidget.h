#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h" // Needed to check Team / Self
#include "KillfeedWidget.generated.h"

class UVerticalBox;
class UTexture2D;

// --- 1. THE DATA STRUCT ---
USTRUCT(BlueprintType)
struct FKillfeedData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Killfeed")
	FString KillerName;

	UPROPERTY(BlueprintReadWrite, Category = "Killfeed")
	APlayerState* KillerPlayerState;

	UPROPERTY(BlueprintReadWrite, Category = "Killfeed")
	FString VictimName;

	UPROPERTY(BlueprintReadWrite, Category = "Killfeed")
	APlayerState* VictimPlayerState;

	UPROPERTY(BlueprintReadWrite, Category = "Killfeed")
	UTexture2D* WeaponIcon;

	UPROPERTY(BlueprintReadWrite, Category = "Killfeed")
	bool bIsHeadshot = false;

	UPROPERTY(BlueprintReadWrite, Category = "Killfeed")
	bool bIsSniper = false;

	UPROPERTY(BlueprintReadWrite, Category = "Killfeed")
	bool bIsNoScope = false;

	UPROPERTY(BlueprintReadWrite, Category = "Killfeed")
	float DistanceMeters = 0.0f;
};

// --- 2. THE ITEM BASE CLASS ---
UCLASS()
class SND_FPS_API UKillfeedItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// This pushes the C++ struct directly into your Blueprint item!
	UFUNCTION(BlueprintImplementableEvent, Category = "Killfeed")
	void SetupKillfeedItem(FKillfeedData KillData);
};

// --- CONTAINER CLASS ---
UCLASS()
class SND_FPS_API UKillfeedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Call this when someone dies to generate a new message */
	UFUNCTION(BlueprintCallable, Category = "Killfeed")
	void AddKillMessage(FKillfeedData KillData);

protected:
	/** The Vertical Box that holds the messages */
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* MessageContainer;

	/** Notice we changed this to look for our specific Item class! */
	UPROPERTY(EditDefaultsOnly, Category = "Killfeed Config")
	TSubclassOf<UKillfeedItemWidget> MessageWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Killfeed Config")
	int32 MaxMessages = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Killfeed Config")
	float MessageDisplayTime = 10.0f;

private:
	// --- THE TIMER SYSTEM ---
	void RemoveSpecificMessage(TWeakObjectPtr<UKillfeedItemWidget> MessageToRemove);
};
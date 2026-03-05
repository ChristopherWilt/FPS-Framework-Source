#pragma once

#include "CoreMinimal.h"
#include "WeaponEnums.generated.h"

// --- EXISTING ENUMS ---
UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	Melee		UMETA(DisplayName = "Melee"),
	Sidearm		UMETA(DisplayName = "Sidearm"),
	Primary		UMETA(DisplayName = "Primary"),
	None		UMETA(DisplayName = "None")
};

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Single		UMETA(DisplayName = "Single Fire"),
	Burst		UMETA(DisplayName = "Burst Fire"),
	Automatic	UMETA(DisplayName = "Automatic")
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Melee		UMETA(DisplayName = "Melee"),
	Sidearm		UMETA(DisplayName = "Pistol"),
	SMG			UMETA(DisplayName = "SMG"),
	Shotgun		UMETA(DisplayName = "Shotgun"),
	Rifle		UMETA(DisplayName = "Assault Rifle"),
	Sniper		UMETA(DisplayName = "Sniper Rifle")
};

// --- RECOIL SYSTEM ENUM ---
UENUM(BlueprintType)
enum class ERecoilSystem : uint8
{
	Procedural  UMETA(DisplayName = "Procedural (CoD Style)"),
	Pattern     UMETA(DisplayName = "Pattern (Valorant/CS Style)")
};

USTRUCT(BlueprintType)
struct FWeaponFireConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Fire Mode")
	bool bCanSingle = true;

	UPROPERTY(EditDefaultsOnly, Category = "Fire Mode")
	bool bCanBurst = false;

	UPROPERTY(EditDefaultsOnly, Category = "Fire Mode")
	bool bCanAuto = true;

	UPROPERTY(EditDefaultsOnly, Category = "Fire Mode")
	int32 BurstCount = 3;
};
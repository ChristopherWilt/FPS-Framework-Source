#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponEnums.h"
#include "WeaponRecoilData.h"
#include "Weapon.generated.h"

class UNiagaraSystem;
class UAnimSequence;

UCLASS()
class SND_FPS_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();

	// --- GAMEPLAY STATE ---
	bool ConsumeAmmo(int32 Amount = 1);
	void ReloadAmmo();

	bool HasAmmo() const;
	bool IsMagFull() const;
	bool CanReload() const;

	// --- FIRE MODES ---
	void CycleFireMode();
	EFireMode GetCurrentFireMode() const { return CurrentFireMode; }
	FWeaponFireConfig GetFireConfig() const { return FireConfig; }

	// --- VISUALS ---
	void PlayFireEffects(const FHitResult& Hit);

	// --- GETTERS ---
	FORCEINLINE UStaticMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	FORCEINLINE UStaticMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; }

	// - Ammo Getters for HUD -
	UFUNCTION(BlueprintPure, Category = "Ammo")
	int32 GetCurrentMagAmmo() const { return CurrentMagAmmo; }
	UFUNCTION(BlueprintPure, Category = "Ammo")
	int32 GetCurrentReserveAmmo() const { return CurrentReserveAmmo; }

	FORCEINLINE UAnimSequence* GetFireAnim1P() const { return FireAnimation_1P; }
	FORCEINLINE UAnimSequence* GetFireAnim3P() const { return FireAnimation_3P; }
	FORCEINLINE UAnimSequence* GetReloadTacAnim() const { return ReloadTacAnim; }
	FORCEINLINE UAnimSequence* GetReloadDryAnim() const { return ReloadDryAnim; }
	FORCEINLINE UAnimSequence* GetInspectAnim() const { return InspectAnim; }
	FORCEINLINE UAnimSequence* GetReloadTacAnim3P() const { return ReloadTacAnim_3P; }
	FORCEINLINE UAnimSequence* GetReloadDryAnim3P() const { return ReloadDryAnim_3P; }
	FORCEINLINE UAnimSequence* GetInspectAnim3P() const { return InspectAnim_3P; }
	FORCEINLINE class UAnimationAsset* GetIdleAnimation() const { return IdleAnimation; }

	// --- PROPERTIES ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float FireRate = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float WeaponDamage = 25.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float WeaponRange = 5000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Stats")
	EWeaponType WeaponType;

	// UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Data")
	FText WeaponName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Data")
	int32 WeaponCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI Data")
	class UTexture2D* WeaponIcon;

	// Offsets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Stats")
	FVector AttachmentOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Stats")
	FRotator AttachmentRotation;

	// --- OFFSETS & ADS ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Stats|Positioning")
	FVector HipOffset;

	// 100% AUTOMATED: The code fills this in instantly on equip
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Stats|Positioning")
	FVector ADSOffset;

	// 100% AUTOMATED: The code fills this in instantly on equip
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Stats|Positioning")
	FRotator ADSRotation;

	// --- SIGHT ALIGNMENT LOGIC ---
	UFUNCTION(BlueprintPure, Category = "Weapon Stats|Positioning")
	FVector GetActiveSightSocketLocation() const;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* AttachedOpticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Stats|Positioning")
	float AimFOV = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Stats|Positioning")
	float AimSpeed = 20.0f; // Higher = Snappier transition to screen center


	// --- DROPPING & PICKUP ---
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon State")
	bool bIsDropped = false;

	void DropWeapon();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DropWeapon();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PickupWeapon();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* InteractSphere;

protected:
	virtual void BeginPlay() override;

	// --- AMMO ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	int32 MaxMagCapacity = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	int32 StartReserveAmmo = 120;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ammo")
	int32 CurrentMagAmmo;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ammo")
	int32 CurrentReserveAmmo;

	// --- FIRE MODE CONFIG ---
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	FWeaponFireConfig FireConfig;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	EFireMode CurrentFireMode;

	// --- ANIMATIONS ---
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* FireAnimation_1P;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* FireAnimation_3P;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* ReloadTacAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* ReloadTacAnim_3P;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* ReloadDryAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* ReloadDryAnim_3P;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* InspectAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* InspectAnim_3P;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimationAsset* IdleAnimation;

	// --- FX ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX")
	UNiagaraSystem* MuzzleFlashEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX")
	UNiagaraSystem* TracerEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX")
	UNiagaraSystem* BarrelSmokeEffect;

	// -- IMPACT FX --
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX|Impact")
	UNiagaraSystem* ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX|Impact")
	UMaterialInterface* ImpactDecalMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX|Impact")
	FVector DecalSize = FVector(8.0f, 8.0f, 8.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX|Impact")
	float DecalLifeSpan = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX|Impact")
	float DecalFadeScreenSize = 0.001f;

	// --- MESHES ---
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* FirstPersonMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ThirdPersonMesh;

	// --- NETWORKING FX ---
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireFX(const FHitResult& Hit);

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX")
	float TracerSpeed = 10000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon Stats|Recoil")
	UWeaponRecoilData* RecoilData;
};
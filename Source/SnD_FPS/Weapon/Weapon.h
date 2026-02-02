#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponEnums.h"
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
	/** Consumes ammo. Returns true if successful. */
	bool ConsumeAmmo(int32 Amount = 1);
	/** Adds ammo from reserve to mag. */
	void ReloadAmmo();

	// Checks
	bool HasAmmo() const;
	bool IsMagFull() const;
	bool CanReload() const;

	// --- FIRE MODES ---
	void CycleFireMode();
	EFireMode GetCurrentFireMode() const { return CurrentFireMode; }
	FWeaponFireConfig GetFireConfig() const { return FireConfig; }

	// --- VISUALS ---
	/** Called by Character to play FX (Muzzle Flash, etc) */
	void PlayFireEffects(FVector TraceEnd);


	// --- GETTERS ---
	FORCEINLINE UStaticMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	FORCEINLINE UStaticMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; }

	// Animation Getters
	FORCEINLINE UAnimSequence* GetFireAnim1P() const { return FireAnimation_1P; }
	FORCEINLINE UAnimSequence* GetFireAnim3P() const { return FireAnimation_3P; }
	FORCEINLINE UAnimSequence* GetReloadTacAnim() const { return ReloadTacAnim; }
	FORCEINLINE UAnimSequence* GetReloadDryAnim() const { return ReloadDryAnim; }
	FORCEINLINE UAnimSequence* GetInspectAnim() const { return InspectAnim; }
	FORCEINLINE class UAnimationAsset* GetIdleAnimation() const { return IdleAnimation; }

	// --- PROPERTIES ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float FireRate = 600.f; // RPM

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
	UAnimSequence* ReloadTacAnim; // Ammo left

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* ReloadDryAnim; // Empty

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimSequence* InspectAnim;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimationAsset* IdleAnimation;

	// --- FX ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX")
	UNiagaraSystem* MuzzleFlashEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX")
	UNiagaraSystem* TracerEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX")
	UNiagaraSystem* BarrelSmokeEffect;

	// --- MESHES ---
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* FirstPersonMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ThirdPersonMesh;

	// --- NETWORKING FX ---
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireFX(FVector MuzzleLocation, FVector TraceEnd);

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FX")
	float TracerSpeed = 10000.f;
};
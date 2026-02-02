// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyAnimInstance.generated.h"

class APlayerCharacter;
class AWeapon;

UCLASS()
class SND_FPS_API UMyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// --- WEAPON ANIMATION STATE ---

	/** (Restored Name) True if we have a weapon equipped */
	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	bool bHasWeaponAnimation;

	/** (Restored Name) The specific idle pose for the current gun */
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Playback")
	UAnimSequence* CurrentIdleAnimation;

	/** * NOTE: 'bIsFiring' is REMOVED intentionally.
	 * With Dynamic Montages, we don't use a boolean to play fire anims anymore.
	 * The C++ code in PlayerCharacter pushes the animation directly when you click.
	 */

	 // --- MOVEMENT DATA (For AnimGraph) ---
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float Speed;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Movement")
	float Direction;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	bool bIsCrouching;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	bool bIsSliding;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|State")
	bool bIsAccelerating;

	// --- AIM OFFSETS (For looking up/down) ---
	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aiming")
	float AimPitch;

	UPROPERTY(BlueprintReadOnly, Category = "Animation|Aiming")
	float AimYaw;

private:
	UPROPERTY()
	APlayerCharacter* PlayerCharacter;

	// (Restored Helper) Helper to get current weapon safely
	UFUNCTION(BlueprintPure, Category = "Helper")
	AWeapon* GetCurrentWeapon() const;
};
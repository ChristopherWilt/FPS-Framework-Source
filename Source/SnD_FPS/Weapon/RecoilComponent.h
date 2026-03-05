#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SnD_FPS/Weapon/WeaponRecoilData.h"
#include "RecoilComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SND_FPS_API URecoilComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URecoilComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Recoil_Fire(UWeaponRecoilData* Data);

	UFUNCTION(BlueprintPure, Category = "Recoil")
	FTransform GetCurrentVisualRecoil() const;

	float ApplyRecoilCompensation(float InputPitch);

private:
	// --- SEPARATE LOGIC HANDLERS ---
	void ApplyProceduralRecoil(UWeaponRecoilData* Data);
	void ApplyPatternRecoil(UWeaponRecoilData* Data);

	UPROPERTY()
	UWeaponRecoilData* CurrentData;

	// --- CAMERA RECOIL STATE ---
	FRotator CurrentCameraRecoil;
	FRotator TargetCameraRecoil;
	float RecoilRecoveryTime;

	// --- VISUAL RECOIL STATE ---
	FVector CurrentVisualTrans;
	FVector TargetVisualTrans;
	FVector VisualTransVelocity;

	FRotator CurrentVisualRot;
	FRotator TargetVisualRot;
	FRotator VisualRotVelocity;

	// --- CAMERA ROLL STATE ---
	float CurrentCameraRoll = 0.0f;
	float TargetCameraRoll = 0.0f;
	double CameraRollVelocity = 0.0f;

	UPROPERTY()
	class UCameraComponent* PlayerCamera;

	// --- STATE ---
	int32 ShotsFired = 0;
	float StochasticDirection = 1.0f;
	int32 ShotsInDirection = 0;

	float SpringInterp(float Current, float Target, double& Velocity, float Stiffness, float Damping, float DeltaTime);
};
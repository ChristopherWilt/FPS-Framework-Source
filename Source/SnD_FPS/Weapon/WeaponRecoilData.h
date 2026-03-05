#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Curves/CurveVector.h"
#include "Camera/CameraShakeBase.h"
#include "SnD_FPS/Weapon/WeaponEnums.h"
#include "WeaponRecoilData.generated.h"

UCLASS(BlueprintType)
class SND_FPS_API UWeaponRecoilData : public UDataAsset
{
	GENERATED_BODY()

public:
	// --- SYSTEM SELECTION ---
	UPROPERTY(EditAnywhere, Category = "System Configuration")
	ERecoilSystem RecoilSystem = ERecoilSystem::Procedural;

	// ==============================================================================
	// 1. PROCEDURAL RECOIL (CoD/Valorant Style Defaults)
	// ==============================================================================
	// Tightened the kick cone significantly so the gun stays on target better
	UPROPERTY(EditAnywhere, Category = "Procedural Settings", meta = (EditCondition = "RecoilSystem == ERecoilSystem::Procedural", EditConditionHides))
	float VerticalKickMin = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Procedural Settings", meta = (EditCondition = "RecoilSystem == ERecoilSystem::Procedural", EditConditionHides))
	float VerticalKickMax = 0.35f;

	// The maximum angle the camera can be forced upward. Prevents "sky-gazing" after a long spray.
	UPROPERTY(EditAnywhere, Category = "Procedural Settings", meta = (EditCondition = "RecoilSystem == ERecoilSystem::Procedural", EditConditionHides))
	float MaxProceduralRecoilPitch = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Procedural Settings", meta = (EditCondition = "RecoilSystem == ERecoilSystem::Procedural", EditConditionHides))
	float HorizontalKickRange = 0.1f;

	// ==============================================================================
	// 2. PATTERN RECOIL (CS/Valorant Style)
	// ==============================================================================
	UPROPERTY(EditAnywhere, Category = "Pattern Settings", meta = (EditCondition = "RecoilSystem == ERecoilSystem::Pattern", EditConditionHides))
	UCurveVector* RecoilPatternCurve;

	UPROPERTY(EditAnywhere, Category = "Pattern Settings", meta = (EditCondition = "RecoilSystem == ERecoilSystem::Pattern", EditConditionHides))
	int32 PatternApexShot = 10;

	UPROPERTY(EditAnywhere, Category = "Pattern Settings", meta = (EditCondition = "RecoilSystem == ERecoilSystem::Pattern", EditConditionHides))
	int32 CameraFreezeShot = 5;

	UPROPERTY(EditAnywhere, Category = "Pattern Settings|Stochastic", meta = (EditCondition = "RecoilSystem == ERecoilSystem::Pattern", EditConditionHides))
	float StochasticHorizontalRange = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Pattern Settings|Stochastic", meta = (EditCondition = "RecoilSystem == ERecoilSystem::Pattern", EditConditionHides))
	float StochasticVerticalJitter = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Pattern Settings", meta = (EditCondition = "RecoilSystem == ERecoilSystem::Pattern", EditConditionHides))
	bool bFreezeCameraAtApex = true;

	// ==============================================================================
	// 3. SHARED SETTINGS (Snappy Interp & Recovery)
	// ==============================================================================
	// High InterpSpeed means the recoil snaps instantly (responsive).
	UPROPERTY(EditAnywhere, Category = "Shared Settings")
	float InterpSpeed = 25.0f;

	// How fast the crosshair returns to the center after you stop shooting
	UPROPERTY(EditAnywhere, Category = "Shared Settings")
	float RecoverySpeed = 15.0f;

	// Very short delay so tapping feels accurate
	UPROPERTY(EditAnywhere, Category = "Shared Settings")
	float RecoveryDelay = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Shared Settings")
	bool bEnableRecoilRecovery = true;

	// ==============================================================================
	// 4. VISUAL RECOIL (The "Gun Punch" - Keeps the gun feeling like solid metal)
	// ==============================================================================
	UPROPERTY(EditAnywhere, Category = "Visual Recoil")
	float KickbackAmount = -0.5f; // Small backward punch

	UPROPERTY(EditAnywhere, Category = "Visual Recoil")
	float VisualMuzzleRise = 0.5f; // Slight tilt up in the hands

	UPROPERTY(EditAnywhere, Category = "Visual Recoil")
	float VisualRollAmplitude = 0.1f; // Tiny twist

	// High stiffness/damping acts like a tight metal spring. 
	// Stops the gun from wobbling like jelly.
	UPROPERTY(EditAnywhere, Category = "Visual Recoil")
	float VisualStiffness = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Visual Recoil")
	float VisualDamping = 15.0f;

	/** CAMERA FEEL */
	UPROPERTY(EditAnywhere, Category = "Camera Feel")
	TSubclassOf<UCameraShakeBase> CameraShake;

	UPROPERTY(EditAnywhere, Category = "Camera Feel")
	float CameraRollKick = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Camera Feel")
	float CameraStiffness = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Camera Feel")
	float CameraDamping = 15.0f;
};
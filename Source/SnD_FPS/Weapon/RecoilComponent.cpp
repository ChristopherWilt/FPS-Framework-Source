#include "SnD_FPS/Weapon/RecoilComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"

URecoilComponent::URecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URecoilComponent::Recoil_Fire(UWeaponRecoilData* Data)
{
	if (!Data) return;
	CurrentData = Data;
	ShotsFired++;

	// 1. CHOOSE SYSTEM
	if (Data->RecoilSystem == ERecoilSystem::Pattern)
	{
		ApplyPatternRecoil(Data);
	}
	else
	{
		ApplyProceduralRecoil(Data);
	}

	// 2. SHARED EFFECTS (Camera Shake & Roll)
	float RollDir = (FMath::RandBool()) ? 1.0f : -1.0f;
	TargetCameraRoll += (Data->CameraRollKick * RollDir);

	APlayerController* PC = Cast<APlayerController>(Cast<ACharacter>(GetOwner())->GetController());
	if (PC && Data->CameraShake)
	{
		PC->ClientStartCameraShake(Data->CameraShake, 1.0f);
	}

	RecoilRecoveryTime = 0.0f;
}

// ==========================================================
// SYSTEM 1: PROCEDURAL (CoD)
// ==========================================================
void URecoilComponent::ApplyProceduralRecoil(UWeaponRecoilData* Data)
{
	// 1. Calculate Random Kick
	float VertKick = FMath::RandRange(Data->VerticalKickMin, Data->VerticalKickMax);
	float HorizKick = FMath::RandRange(-Data->HorizontalKickRange, Data->HorizontalKickRange);

	// 2. Apply to Camera (WITH CLAMP)
	// We assume TargetCameraRecoil.Pitch is negative (Upward). 
	// We want to limit how "negative" it gets.
	float NewPitch = TargetCameraRecoil.Pitch - VertKick;

	// Clamp between -MaxRecoil and 0. 
	// Note: Pitch is usually negative for "Up" in Controller input, but let's check your specific logic. 
	// If your inputs use -= VertKick, then the value is becoming more negative.
	if (NewPitch >= -Data->MaxProceduralRecoilPitch)
	{
		TargetCameraRecoil.Pitch = NewPitch;
	}
	else
	{
		// We hit the ceiling. Don't add more vertical recoil.
		// Optionally, you can add horizontal kick only here for a "bouncing at the top" feel.
		TargetCameraRecoil.Pitch = -Data->MaxProceduralRecoilPitch;
	}

	// Horizontal adds freely (or you can clamp this too if you want)
	TargetCameraRecoil.Yaw += HorizKick;

	// 3. Visual Kick (Gun Model) - Unaffected by camera clamp (Gun keeps shaking!)
	TargetVisualTrans.X += Data->KickbackAmount;

	float VisualPitch = TargetVisualRot.Pitch + Data->VisualMuzzleRise;
	TargetVisualRot.Pitch = FMath::Clamp(VisualPitch, 0.0f, 20.0f); // Keep the visual clamp

	TargetVisualRot.Roll += FMath::RandRange(-Data->VisualRollAmplitude, Data->VisualRollAmplitude);
}

// ==========================================================
// SYSTEM 2: PATTERN (Valorant)
// ==========================================================
void URecoilComponent::ApplyPatternRecoil(UWeaponRecoilData* Data)
{
	if (!Data->RecoilPatternCurve) return;

	// Phase A: The Curve (Deterministic)
	if (ShotsFired <= Data->PatternApexShot)
	{
		FVector CurrentStep = Data->RecoilPatternCurve->GetVectorValue((float)ShotsFired);
		FVector PreviousStep = (ShotsFired > 1) ? Data->RecoilPatternCurve->GetVectorValue((float)(ShotsFired - 1)) : FVector::ZeroVector;

		FVector Delta = CurrentStep - PreviousStep;

		// --- SPLIT LOGIC ---
		// 1. BEFORE FREEZE: Move Camera Normally
		if (ShotsFired <= Data->CameraFreezeShot)
		{
			TargetCameraRecoil.Pitch -= Delta.X; // Negative = UP
			TargetCameraRecoil.Yaw += Delta.Y;
		}
		// 2. GHOST PHASE (After Freeze, Before Apex): Move Gun Visuals
		else
		{
			// Get the Total Curve Value at the moment the camera froze
			FVector FreezeVal = Data->RecoilPatternCurve->GetVectorValue((float)Data->CameraFreezeShot);

			// Get the Total Curve Value right now
			FVector CurrentVal = Data->RecoilPatternCurve->GetVectorValue((float)ShotsFired);

			// Calculate how much FURTHER the gun should be than the camera
			FVector GhostOffset = CurrentVal - FreezeVal;

			// APPLY ABSOLUTE OFFSET (Assignment '=', not Accumulation '+=' )
			// We negate X because Camera uses Negative for UP.
			// This forces the gun to hold this angle, fighting the recovery.
			TargetVisualRot.Pitch = -GhostOffset.X;
			TargetVisualRot.Yaw = GhostOffset.Y;
		}
	}
	// Phase B: The Apex (Stochastic Sway) - Happens after Shot 10
	else
	{
		if (ShotsInDirection > FMath::RandRange(3, 6))
		{
			StochasticDirection *= -1.0f;
			ShotsInDirection = 0;
		}

		float SwayAmount = Data->StochasticHorizontalRange * StochasticDirection;
		TargetVisualRot.Yaw = SwayAmount;

		TargetVisualRot.Pitch += FMath::RandRange(-Data->StochasticVerticalJitter, Data->StochasticVerticalJitter);

		// Keep the gun pointed up at the Apex height (Ghost Phase End Height)
		// We get the final curve value (Apex) and freeze the pitch there
		FVector ApexVal = Data->RecoilPatternCurve->GetVectorValue((float)Data->PatternApexShot);
		FVector FreezeVal = Data->RecoilPatternCurve->GetVectorValue((float)Data->CameraFreezeShot);
		float ApexPitchOffset = -(ApexVal.X - FreezeVal.X);

		// Clamp so it stays roughly at the apex height
		float NewPitch = TargetVisualRot.Pitch;
		// You might want to hard set it to ApexPitchOffset + Jitter to stop it dropping
		TargetVisualRot.Pitch = ApexPitchOffset + FMath::RandRange(-Data->StochasticVerticalJitter, Data->StochasticVerticalJitter);

		ShotsInDirection++;
	}

	// Always apply backward kick
	TargetVisualTrans.X += Data->KickbackAmount;
}

void URecoilComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!CurrentData || !GetOwner()) return;

	APlayerController* PC = Cast<APlayerController>(Cast<ACharacter>(GetOwner())->GetController());

	// -----------------------------------------------------------------------
	// CAMERA RECOIL RESOLVER
	// -----------------------------------------------------------------------
	if (PC && PC->IsLocalController())
	{
		RecoilRecoveryTime += DeltaTime;

		// 1. RECOVERY LOGIC
		if (RecoilRecoveryTime > CurrentData->RecoveryDelay)
		{
			ShotsFired = 0;
			ShotsInDirection = 0;

			// Stop momentum if fully recovered
			if (RecoilRecoveryTime > CurrentData->RecoveryDelay + 0.5f)
			{
				VisualRotVelocity = FRotator::ZeroRotator;
				VisualTransVelocity = FVector::ZeroVector;
			}

			if (CurrentData->bEnableRecoilRecovery)
			{
				// Smoothly return Target to Zero
				TargetCameraRecoil = FMath::RInterpTo(TargetCameraRecoil, FRotator::ZeroRotator, DeltaTime, CurrentData->RecoverySpeed);
			}
		}

		// 2. INTERPOLATION (Current -> Target)
		FRotator NewRecoilRotation = FMath::RInterpTo(CurrentCameraRecoil, TargetCameraRecoil, DeltaTime, CurrentData->InterpSpeed);
		FRotator DeltaRot = NewRecoilRotation - CurrentCameraRecoil;

		// 3. APPLY INPUT (With Freeze Check for Pattern Mode)
		bool bCanApplyInput = true;

		if (CurrentData->RecoilSystem == ERecoilSystem::Pattern && CurrentData->bFreezeCameraAtApex)
		{
			bool bIsRecovering = (RecoilRecoveryTime > CurrentData->RecoveryDelay);

			// Use the new CameraFreezeShot variable
			bool bPastFreeze = (ShotsFired > CurrentData->CameraFreezeShot);

			if (bPastFreeze && !bIsRecovering)
			{
				bCanApplyInput = false;
			}
		}

		if (bCanApplyInput)
		{
			PC->AddPitchInput(DeltaRot.Pitch);
			PC->AddYawInput(DeltaRot.Yaw);
		}

		CurrentCameraRecoil = NewRecoilRotation;
	}

	// -----------------------------------------------------------------------
	// VISUAL SPRING SOLVERS (Weapon Model)
	// -----------------------------------------------------------------------

	// Always try to return visual targets to zero
	TargetVisualRot.Yaw = FMath::FInterpTo(TargetVisualRot.Yaw, 0.0f, DeltaTime, CurrentData->RecoverySpeed);
	TargetVisualRot.Pitch = FMath::FInterpTo(TargetVisualRot.Pitch, 0.0f, DeltaTime, CurrentData->RecoverySpeed);
	TargetVisualTrans = FMath::VInterpTo(TargetVisualTrans, FVector::ZeroVector, DeltaTime, CurrentData->RecoverySpeed);

	// Spring Calculations
	CurrentVisualTrans.X = SpringInterp(CurrentVisualTrans.X, TargetVisualTrans.X, VisualTransVelocity.X, CurrentData->VisualStiffness, CurrentData->VisualDamping, DeltaTime);

	CurrentVisualRot.Pitch = SpringInterp(CurrentVisualRot.Pitch, TargetVisualRot.Pitch, VisualRotVelocity.Pitch, CurrentData->VisualStiffness, CurrentData->VisualDamping, DeltaTime);
	CurrentVisualRot.Yaw = SpringInterp(CurrentVisualRot.Yaw, TargetVisualRot.Yaw, VisualRotVelocity.Yaw, CurrentData->VisualStiffness, CurrentData->VisualDamping, DeltaTime);
	CurrentVisualRot.Roll = SpringInterp(CurrentVisualRot.Roll, TargetVisualRot.Roll, VisualRotVelocity.Roll, CurrentData->VisualStiffness, CurrentData->VisualDamping, DeltaTime);

	// Camera Roll
	TargetCameraRoll = FMath::FInterpTo(TargetCameraRoll, 0.0f, DeltaTime, CurrentData->RecoverySpeed);
	CurrentCameraRoll = SpringInterp(CurrentCameraRoll, TargetCameraRoll, CameraRollVelocity, CurrentData->CameraStiffness, CurrentData->CameraDamping, DeltaTime);

	if (PlayerCamera)
	{
		FRotator NewRot = PlayerCamera->GetRelativeRotation();
		NewRot.Roll = CurrentCameraRoll;
		PlayerCamera->SetRelativeRotation(NewRot);
	}
}

float URecoilComponent::SpringInterp(float Current, float Target, double& Velocity, float Stiffness, float Damping, float DeltaTime)
{
	float Displacement = Current - Target;
	float Force = (-Stiffness * Displacement) - (Damping * (float)Velocity);
	Velocity += Force * DeltaTime;
	return Current + ((float)Velocity * DeltaTime);
}

FTransform URecoilComponent::GetCurrentVisualRecoil() const
{
	return FTransform(CurrentVisualRot, CurrentVisualTrans, FVector::OneVector);
}

float URecoilComponent::ApplyRecoilCompensation(float InputPitch)
{
	// 1. If we aren't recoiling, just return the raw input to the controller
	if (FMath::IsNearlyZero(TargetCameraRecoil.Pitch, 0.01f))
	{
		return InputPitch;
	}

	// 2. Check if the Input opposes the Recoil
	// Assuming: Recoil moves Pitch Negative (Up), Input moves Pitch Positive (Down)
	bool bIsFightingRecoil = (TargetCameraRecoil.Pitch < 0.0f && InputPitch > 0.0f) ||
		(TargetCameraRecoil.Pitch > 0.0f && InputPitch < 0.0f);

	if (bIsFightingRecoil)
	{
		float OldRecoil = TargetCameraRecoil.Pitch;

		// Add the input to the recoil (Fighting it means bringing it closer to 0)
		float NewRecoil = OldRecoil + InputPitch;

		// 3. Did we overshoot 0? (Pulled down more than the gun kicked up)
		bool bCrossedZero = (OldRecoil < 0.0f && NewRecoil > 0.0f) ||
			(OldRecoil > 0.0f && NewRecoil < 0.0f);

		if (bCrossedZero)
		{
			// We fully canceled the recoil, and have some input left over
			TargetCameraRecoil.Pitch = 0.0f;

			// Return the "leftover" input to move the camera normally
			return NewRecoil;
		}
		else
		{
			// We absorbed ALL the input into the recoil compensation
			TargetCameraRecoil.Pitch = NewRecoil;

			// Return 0 so the Camera Controller doesn't move essentially "eating" the input
			return 0.0f;
		}
	}

	// If we are adding TO the recoil (looking up while kicking up), just let it happen
	return InputPitch;
}
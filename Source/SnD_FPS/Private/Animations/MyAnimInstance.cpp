#include "Animations/MyAnimInstance.h"
#include "SnD_FPS/Player/PlayerCharacter.h"
#include "SnD_FPS/Weapon/Weapon.h"
#include "SnD_FPS/Weapon/InventoryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UMyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	PlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
}

void UMyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Safe check
	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
	}
	if (!PlayerCharacter) return;

	// --------------------------------------------------------
	// 1. MOVEMENT CALCULATIONS
	// --------------------------------------------------------
	FVector Velocity = PlayerCharacter->GetVelocity();
	Velocity.Z = 0;
	Speed = Velocity.Size();

	// Calculate Direction for Strafe blending
	FRotator VelocityRot = Velocity.Rotation();
	FRotator ActorRot = PlayerCharacter->GetActorRotation();
	Direction = UKismetMathLibrary::NormalizedDeltaRotator(VelocityRot, ActorRot).Yaw;

	// Acceleration check
	bIsAccelerating = PlayerCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0;
	bIsInAir = PlayerCharacter->GetCharacterMovement()->IsFalling();
	bIsCrouching = PlayerCharacter->IsCrouching();
	bIsSliding = PlayerCharacter->bIsSliding;

	// --------------------------------------------------------
	// 2. WEAPON STATE (Using Helper)
	// --------------------------------------------------------
	AWeapon* CurrentWeapon = GetCurrentWeapon(); // <--- Restored usage

	bHasWeaponAnimation = (CurrentWeapon != nullptr);

	if (CurrentWeapon)
	{
		// Pull the correct Idle animation (Pistol vs Rifle) from the Character
		CurrentIdleAnimation = PlayerCharacter->GetCurrentWeaponIdleAnimation();
	}
	else
	{
		CurrentIdleAnimation = nullptr;
	}

	// --------------------------------------------------------
	// 3. AIM OFFSETS
	// --------------------------------------------------------
	FRotator AimRotation = PlayerCharacter->GetBaseAimRotation();
	FRotator MovementRotation = PlayerCharacter->GetActorRotation();
	FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, MovementRotation);

	AimPitch = Delta.Pitch;
	AimYaw = Delta.Yaw;
}

// (Restored Helper Implementation)
AWeapon* UMyAnimInstance::GetCurrentWeapon() const
{
	APlayerCharacter* PCharacter = Cast<APlayerCharacter>(TryGetPawnOwner());
	if (PCharacter && PCharacter->GetInventoryComponent())
	{
		return PCharacter->GetInventoryComponent()->GetCurrentWeapon();
	}
	return nullptr;
}
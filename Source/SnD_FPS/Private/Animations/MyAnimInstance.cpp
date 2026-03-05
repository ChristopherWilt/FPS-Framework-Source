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
	PlayerCharacter->GetisAiming(bIsAiming);

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

	// --- IK LOGIC ---

	// 1. Determine if we should use IK (Not melee, and we have a valid weapon)
	bool bUseIK = CurrentWeapon && CurrentWeapon->WeaponType != EWeaponType::Melee && !PlayerCharacter->IsActionHappening();

	IKLeftHandAlpha = FMath::FInterpTo(IKLeftHandAlpha, bUseIK ? 1.0f : 0.0f, DeltaSeconds, 10.0f);

	if (bUseIK && PlayerCharacter)
	{
		// 2. Find out which mesh we are animating (1P or 3P?)
		USkeletalMeshComponent* OwningMesh = GetOwningComponent();
		bool bIsFirstPerson = (OwningMesh == PlayerCharacter->GetFirstPersonMesh());

		// 3. Get the correct Weapon Mesh (1P weapon for 1P view, 3P weapon for 3P view)
		UStaticMeshComponent* TargetWeaponMesh = bIsFirstPerson ? CurrentWeapon->GetFirstPersonMesh() : CurrentWeapon->GetThirdPersonMesh();

		if (TargetWeaponMesh)
		{
			// 4. Get Socket Transform in World Space
			// MAKE SURE YOUR WEAPON MESH HAS A SOCKET NAMED "LeftHandSocket"
			FTransform SocketTransform = TargetWeaponMesh->GetSocketTransform(FName("LeftHandSocket"), RTS_World);

			// 5. Convert to Component Space (Relative to the Character Mesh) for the AnimGraph
			FTransform MeshTransform = OwningMesh->GetComponentTransform();
			IKLeftHandTransform = SocketTransform.GetRelativeTransform(MeshTransform);
		}
	}

	// --------------------------------------------------------
	// 3. AIM OFFSETS
	// --------------------------------------------------------
	FRotator AimRotation = PlayerCharacter->GetBaseAimRotation();
	FRotator MovementRotation = PlayerCharacter->GetActorRotation();
	FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(AimRotation, MovementRotation);

	AimPitch = Delta.Pitch;
	AimYaw = Delta.Yaw;

	if (PlayerCharacter && PlayerCharacter->GetRecoilComponent())
	{
		// We override this to Identity (Zero) because the PlayerCharacter.cpp 
		// now handles all Visual Recoil perfectly in pure Camera Space!
		VisualRecoilTransform = FTransform::Identity;
	}
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
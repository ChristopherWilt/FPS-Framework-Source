#include "SnD_FPS/Player/PlayerCharacter.h"
#include "SnD_FPS/Weapon/Weapon.h"
#include "SnD_FPS/UI/BuySystem/WeaponBuyMenuWidget.h"
#include "SnD_FPS/UI/Player/ModularPlayerHUD.h"
#include "SnD_FPS/UI/Player/CrosshairWidget.h"
#include "SnD_FPS/UI/Player/CompassWidget.h"
#include "SnD_FPS/UI/Player/WeaponInventoryWidget.h"
#include "SnD_FPS/UI/Player/KillfeedWidget.h"
#include "SnD_FPS/UI/Player/MinimapWidget.h"
#include "SnD_FPS/UI/Player/InteractPromptWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "SnD_FPS/Weapon/InventoryComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PlayerState.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	StartingMoney = 800;
	CurrentMoney = StartingMoney;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	RecoilComp = CreateDefaultSubobject<URecoilComponent>(TEXT("RecoilComponent"));

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(GetCapsuleComponent());
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->SetRelativeLocation(FVector(-15.0f, 0.0f, 74.0f));

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->FieldOfView = DefaultFOV;

	GetMesh()->bCastDynamicShadow = true;
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.0f, 0.f));

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(CameraComponent);

	SetMeshVisibility();
}

void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APlayerCharacter, CurrentHealth);
	DOREPLIFETIME(APlayerCharacter, CurrentMoney);
	DOREPLIFETIME(APlayerCharacter, bIsCrouching);
	DOREPLIFETIME(APlayerCharacter, bActionHappening);
	DOREPLIFETIME(APlayerCharacter, bIsSliding);
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	SetMeshVisibility();

	// --- NEW: SPAWN THE MASTER HUD ---
	if (IsLocallyControlled() && MasterHUDClass)
	{
		MasterHUD = CreateWidget<UModularPlayerHUD>(Cast<APlayerController>(Controller), MasterHUDClass);
		if (MasterHUD)
		{
			MasterHUD->AddToViewport();

			// --- APPLY INITIAL MINIMAP SETTINGS ---
			if (MasterHUD->Minimap)
			{
				MasterHUD->Minimap->ApplyMinimapSettings(CurrentMinimapConfig);
			}
		}
	}

	DefaultGroundFriction = GetCharacterMovement()->GroundFriction;
	DefaultBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;

	// CAPTURE THE BLUEPRINT'S CUSTOM ARM POSITION
	if (FirstPersonMesh)
	{
		DefaultArmsLocation = FirstPersonMesh->GetRelativeLocation();
		DefaultArmsRotation = FirstPersonMesh->GetRelativeRotation();

		CurrentBaseArmsLocation = DefaultArmsLocation;
		CurrentBaseArmsRotation = DefaultArmsRotation;
	}

	if (HasAuthority()) InventoryComponent->InitializeInventory();
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckForInteractables();

	// --- NEW: FEED CROSSHAIR DATA ---
	if (IsLocallyControlled() && MasterHUD && MasterHUD->Crosshair)
	{
		float SpeedSpread = (GetVelocity().Size() / RunSpeed) * 15.0f;
		float FireSpread = bIsTriggerHeld ? 10.0f : 0.0f;
		float TotalSpread = SpeedSpread + FireSpread;
		MasterHUD->Crosshair->ApplyCrosshairConfig(CurrentCrosshairConfig, TotalSpread);
	}

	// --- FEED INVENTORY HUD DATA ---
	if (IsLocallyControlled() && MasterHUD && MasterHUD->WeaponInventory && InventoryComponent)
	{
		if (AWeapon* CurrentWeapon = InventoryComponent->GetCurrentWeapon())
		{
			// 1. Update Ammo
			MasterHUD->WeaponInventory->UpdateAmmo(CurrentWeapon->GetCurrentMagAmmo(), CurrentWeapon->GetCurrentReserveAmmo());

			// 2. Update Fire Mode
			FWeaponFireConfig FC = CurrentWeapon->GetFireConfig();
			MasterHUD->WeaponInventory->UpdateFireMode(CurrentWeapon->GetCurrentFireMode(), FC.bCanSingle, FC.bCanBurst, FC.bCanAuto);

			// 3. Update Icons 
			MasterHUD->WeaponInventory->UpdateWeaponIcons(CurrentWeapon->WeaponIcon, nullptr, nullptr, nullptr);
		}
	}

	// --- FEED COMPASS DATA ---
	if (IsLocallyControlled() && MasterHUD && MasterHUD->Compass)
	{
		float RawYaw = CameraComponent->GetComponentRotation().Yaw;

		// 1. Math for the sliding strip (Normalized to strictly -180 to 180)
		FRotator NormRot = FRotator(0, RawYaw, 0);
		NormRot.Normalize();
		float StripYaw = NormRot.Yaw;

		// 2. Math for the Center Text (Strictly 0 to 359)
		int32 DisplayYaw = FMath::RoundToInt(RawYaw) % 360;
		if (DisplayYaw < 0) DisplayYaw += 360;

		// Placeholder location (You can hook this up to Trigger Volumes later!)
		FString CurrentLocation = TEXT("STREET (WEST)");

		MasterHUD->Compass->UpdateCompass(StripYaw, DisplayYaw, CurrentLocation);
	}

	// --- FEED MINIMAP DATA ---
	if (IsLocallyControlled() && MasterHUD && MasterHUD->Minimap)
	{
		// 1. Get X and Y location of the player
		FVector2D PlayerLoc(GetActorLocation().X, GetActorLocation().Y);

		// 2. Get Camera Yaw so the map rotates when we look around
		float PlayerYaw = CameraComponent->GetComponentRotation().Yaw;

		// 3. Send Location, Yaw, and dynamic FOV to the Minimap
		MasterHUD->Minimap->UpdateMinimap(PlayerLoc, PlayerYaw, CameraComponent->FieldOfView);
	}

	// =========================================================================
	// PROCEDURAL ARMS ALIGNMENT & RECOIL (ADS SYSTEM)
	// =========================================================================
	if (IsLocallyControlled() && InventoryComponent)
	{
		AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
		if (Weapon && Weapon->GetFirstPersonMesh())
		{
			// 1. INTERPOLATE CAMERA FOV
			float TargetFOV = bIsAiming ? Weapon->AimFOV : DefaultFOV;
			if (CameraComponent)
			{
				float NewFOV = FMath::FInterpTo(CameraComponent->FieldOfView, TargetFOV, DeltaTime, Weapon->AimSpeed);
				CameraComponent->SetFieldOfView(NewFOV);
			}

			// 2. DETERMINE TARGET ARMS LOCATION (Pure Absolute Targets Now)
			FVector TargetArmsLocation = DefaultArmsLocation;
			FRotator TargetArmsRotation = DefaultArmsRotation;

			if (bIsAiming)
			{
				// No more "+=". We use the exact, perfect absolute coordinates.
				TargetArmsLocation = Weapon->ADSOffset;
				TargetArmsRotation = Weapon->ADSRotation;
			}

			// 3. SMOOTH PROCEDURAL TRANSITION (This creates the Call of Duty glide)
			CurrentBaseArmsLocation = FMath::VInterpTo(CurrentBaseArmsLocation, TargetArmsLocation, DeltaTime, Weapon->AimSpeed);
			CurrentBaseArmsRotation = FMath::RInterpTo(CurrentBaseArmsRotation, TargetArmsRotation, DeltaTime, Weapon->AimSpeed);

			// 4. APPLY VISUAL RECOIL IN PURE CAMERA SPACE
			FVector FinalLoc = CurrentBaseArmsLocation;
			FRotator FinalRot = CurrentBaseArmsRotation;

			if (RecoilComp)
			{
				FTransform VisualRecoil = RecoilComp->GetCurrentVisualRecoil();
				FVector RecoilLoc = VisualRecoil.GetLocation();
				FRotator RecoilRot = VisualRecoil.Rotator();

				if (bIsAiming)
				{
					RecoilRot.Pitch *= 0.02f;
					RecoilRot.Yaw *= 0.02f;
				}

				FinalLoc += RecoilLoc;
				FinalRot += RecoilRot;
			}

			FirstPersonMesh->SetRelativeLocationAndRotation(FinalLoc, FinalRot);
		}
	}
}

// =========================================================================
// THE AAA AUTO-CALIBRATOR (Runs Automatically On Equip!)
// =========================================================================
void APlayerCharacter::CalculateWeaponADS(AWeapon* Weapon)
{
	if (!Weapon || !Weapon->GetFirstPersonMesh()) return;

	// 1. FREEZE TIME
	FVector TempLoc = FirstPersonMesh->GetRelativeLocation();
	FRotator TempRot = FirstPersonMesh->GetRelativeRotation();

	FirstPersonMesh->SetRelativeLocationAndRotation(DefaultArmsLocation, DefaultArmsRotation);
	FirstPersonMesh->UpdateComponentToWorld(EUpdateTransformFlags::SkipPhysicsUpdate, ETeleportType::TeleportPhysics);
	Weapon->GetFirstPersonMesh()->UpdateComponentToWorld(EUpdateTransformFlags::SkipPhysicsUpdate, ETeleportType::TeleportPhysics);
	if (Weapon->AttachedOpticMesh) Weapon->AttachedOpticMesh->UpdateComponentToWorld(EUpdateTransformFlags::SkipPhysicsUpdate, ETeleportType::TeleportPhysics);

	// 2. ROTATION (Parallel Barrel Alignment)
	FVector BarrelForward = Weapon->GetFirstPersonMesh()->GetSocketRotation(FName("MuzzleSocket")).Vector();
	FVector CamForward = CameraComponent->GetForwardVector();

	FQuat DeltaRot = FQuat::FindBetweenNormals(BarrelForward, CamForward);
	FQuat CurrentArmsQuat = FirstPersonMesh->GetComponentQuat();
	FQuat TargetArmsQuat = DeltaRot * CurrentArmsQuat;

	FQuat CamQuat = CameraComponent->GetComponentQuat();
	FRotator TargetArmsLocalRot = (CamQuat.Inverse() * TargetArmsQuat).Rotator();

	// NEW: Save the absolute rotation, avoiding gimbal lock addition.
	Weapon->ADSRotation = TargetArmsLocalRot;

	// 3. LOCATION (Sight Centering)
	FVector SightLoc = Weapon->GetActiveSightSocketLocation();
	FVector ArmsRootLoc = FirstPersonMesh->GetComponentLocation();

	FVector LocalSightLoc = CurrentArmsQuat.Inverse().RotateVector(SightLoc - ArmsRootLoc);
	FVector NewSightWorldLoc = ArmsRootLoc + TargetArmsQuat.RotateVector(LocalSightLoc);

	FVector TargetWorldLoc = CameraComponent->GetComponentLocation() + (CamForward * 20.0f);
	FVector WorldDiff = TargetWorldLoc - NewSightWorldLoc;
	FVector LocalDiff = CameraComponent->GetComponentTransform().InverseTransformVectorNoScale(WorldDiff);

	// NEW: Save the absolute location
	Weapon->ADSOffset = DefaultArmsLocation + LocalDiff;

	// 4. RESTORE TIME
	FirstPersonMesh->SetRelativeLocationAndRotation(TempLoc, TempRot);
}

// =========================================================================
// INPUT & MOVEMENT
// =========================================================================

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// --- MOVEMENT ---
		EIC->BindAction(RunAction, ETriggerEvent::Started, this, &APlayerCharacter::StartRunning);
		EIC->BindAction(RunAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopRunning);
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::StartCrouching);
		EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopCrouching);
		EIC->BindAction(SlideAction, ETriggerEvent::Started, this, &APlayerCharacter::StartSliding);
		EIC->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

		// --- COMBAT ---
		EIC->BindAction(FireAction, ETriggerEvent::Started, this, &APlayerCharacter::StartFire);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopFire);
		EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &APlayerCharacter::Reload);
		EIC->BindAction(InspectAction, ETriggerEvent::Started, this, &APlayerCharacter::Inspect);
		EIC->BindAction(FireModeAction, ETriggerEvent::Started, this, &APlayerCharacter::SwitchFireMode);

		// --- INTERACTION ---
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &APlayerCharacter::Interact);

		// --- UI/INVENTORY ---
		EIC->BindAction(OpenBuyMenuAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleBuyMenu);
		EIC->BindAction(EquipMeleeAction, ETriggerEvent::Started, this, &APlayerCharacter::EquipMelee);
		EIC->BindAction(EquipSidearmAction, ETriggerEvent::Started, this, &APlayerCharacter::EquipSidearm);
		EIC->BindAction(EquipPrimaryAction, ETriggerEvent::Started, this, &APlayerCharacter::EquipPrimary);

		// ADS
		if (ADSAction)
		{
			EIC->BindAction(ADSAction, ETriggerEvent::Started, this, &APlayerCharacter::StartADS);
			EIC->BindAction(ADSAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopADS);
		}
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	float RawPitchInput = LookAxisVector.Y * -1.0f;

	if (RecoilComp)
	{
		RawPitchInput = RecoilComp->ApplyRecoilCompensation(RawPitchInput);
	}

	AddControllerPitchInput(RawPitchInput);
	AddControllerYawInput(LookAxisVector.X);
}

void APlayerCharacter::StartRunning()
{
	bWantsToRun = true;
	if (!bIsSliding) GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	Server_StartRunning();
}

void APlayerCharacter::StopRunning()
{
	bWantsToRun = false;
	if (!bIsSliding && !bIsCrouching) GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	Server_StopRunning();
}

void APlayerCharacter::Server_StartRunning_Implementation()
{
	bWantsToRun = true;
	if (!bIsSliding) GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void APlayerCharacter::Server_StopRunning_Implementation()
{
	bWantsToRun = false;
	if (!bIsSliding && !bIsCrouching) GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::StartCrouching()
{
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	Crouch();
	Server_StartCrouching();
}

void APlayerCharacter::StopCrouching() { UnCrouch(); Server_StopCrouching(); }

void APlayerCharacter::Server_StartCrouching_Implementation()
{
	if (!bIsCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
		bIsCrouching = true;
		Crouch();
	}
}

void APlayerCharacter::Server_StopCrouching_Implementation() { if (bIsCrouching) { bIsCrouching = false; UnCrouch(); } }

void APlayerCharacter::StartSliding()
{
	bool bCanSlide = !bIsSliding &&
		GetCharacterMovement()->IsMovingOnGround() &&
		GetCharacterMovement()->Velocity.Size() > (RunSpeed - 50.f);

	if (bCanSlide)
	{
		ApplySlidePhysics();
		Server_StartSliding();

		FVector SlideDir = GetLastMovementInputVector().GetSafeNormal();
		if (SlideDir.IsNearlyZero()) SlideDir = GetActorForwardVector();
		GetCharacterMovement()->Launch(SlideDir * SlideImpulseAmount);

		GetWorldTimerManager().SetTimer(SlideTimerHandle, this, &APlayerCharacter::StopSliding, SlideDuration, false);
	}
}

void APlayerCharacter::ApplySlidePhysics()
{
	bIsSliding = true;

	GetCharacterMovement()->GroundFriction = SlideFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = SlideBrakingDeceleration;
	GetCharacterMovement()->MaxWalkSpeedCrouched = SlideSpeed;

	Crouch();

	if (IsLocallyControlled() && SlideCameraShakeClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->ClientStartCameraShake(SlideCameraShakeClass, 1.0f);
		}
	}
}

void APlayerCharacter::Server_StartSliding_Implementation()
{
	ApplySlidePhysics();
}

void APlayerCharacter::StopSliding()
{
	if (bIsSliding)
	{
		ResetSlidePhysics();
		Server_StopSliding();
		GetWorldTimerManager().ClearTimer(SlideTimerHandle);
	}
}

void APlayerCharacter::ResetSlidePhysics()
{
	bIsSliding = false;

	GetCharacterMovement()->GroundFriction = DefaultGroundFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = DefaultBrakingDeceleration;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;

	UnCrouch();

	if (bWantsToRun)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void APlayerCharacter::Server_StopSliding_Implementation()
{
	ResetSlidePhysics();
}

void APlayerCharacter::CancelSlide() { if (bIsSliding) StopSliding(); }

// =========================================================================
// HEALTH & DAMAGE 
// =========================================================================

void APlayerCharacter::OnRep_CurrentHealth() { OnHealthUpdate(); }

void APlayerCharacter::OnHealthUpdate()
{
	if (IsLocallyControlled())
	{
		FString HealthMessage = FString::Printf(TEXT("My health is now %f"), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, HealthMessage);
	}
	if (HasAuthority() && CurrentHealth <= 0) Destroy();
}

float APlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (HasAuthority())
	{
		float DamageToApply = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		DamageToApply = FMath::Min(CurrentHealth, DamageToApply);
		CurrentHealth -= DamageToApply;

		// --- NEW: DEATH & KILLFEED LOGIC ---
		if (CurrentHealth <= 0.0f)
		{
			// Drop weapon on death
			if (InventoryComponent && InventoryComponent->GetCurrentWeapon())
			{
				InventoryComponent->GetCurrentWeapon()->DropWeapon();
			}

			FKillfeedData KillData;

			// 1. GET THE VICTIM DATA
			KillData.VictimPlayerState = GetPlayerState();
			KillData.VictimName = KillData.VictimPlayerState ? KillData.VictimPlayerState->GetPlayerName() : GetName();

			// 2. GET THE KILLER DATA & MATH
			if (EventInstigator)
			{
				KillData.KillerPlayerState = EventInstigator->GetPlayerState<APlayerState>();
				KillData.KillerName = KillData.KillerPlayerState ? KillData.KillerPlayerState->GetPlayerName() : EventInstigator->GetName();

				// If the killer was another player character, calculate distance and No-Scope
				if (APlayerCharacter* KillerChar = Cast<APlayerCharacter>(EventInstigator->GetPawn()))
				{
					// Convert Unreal Units (cm) to Meters
					KillData.DistanceMeters = FVector::Distance(GetActorLocation(), KillerChar->GetActorLocation()) / 100.0f;

					// Check if they were aiming using your Getter!
					bool bWasAiming = false;
					KillerChar->GetisAiming(bWasAiming);
					KillData.bIsNoScope = !bWasAiming;
				}
			}

			// 3. GET THE WEAPON DATA
			if (AWeapon* Weapon = Cast<AWeapon>(DamageCauser))
			{
				KillData.WeaponIcon = Weapon->WeaponIcon;
				KillData.bIsSniper = (Weapon->WeaponType == EWeaponType::Sniper);
			}

			// 4. CHECK FOR HEADSHOT
			if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
			{
				const FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent;

				// Ensure this matches whatever you named the head bone in your Skeletal Mesh!
				FName HitBone = PointDamageEvent->HitInfo.BoneName;
				if (HitBone == "head" || HitBone == "Head" || HitBone == "neck")
				{
					KillData.bIsHeadshot = true;
				}
			}

			// 5. BROADCAST TO ALL PLAYERS
			Multicast_BroadcastKill(KillData);
		}

		// This handles your existing debug message and the Destroy() call
		OnHealthUpdate();
		return DamageToApply;
	}
	return 0.0f;
}

// =========================================================================
// COMBAT SYSTEM
// =========================================================================

void APlayerCharacter::StartFire()
{
	bIsTriggerHeld = true;
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon) return;

	if (bActionHappening)
	{
		if (Weapon->HasAmmo()) CancelAction();
		else return;
	}

	EFireMode Mode = Weapon->GetCurrentFireMode();
	if (Mode == EFireMode::Single) FireShot();
	else if (Mode == EFireMode::Burst) { BurstShotsRemaining = Weapon->GetFireConfig().BurstCount; FireShot(); }
	else if (Mode == EFireMode::Automatic) FireShot();
}

void APlayerCharacter::StopFire()
{
	bIsTriggerHeld = false;
	if (InventoryComponent->GetCurrentWeapon() && InventoryComponent->GetCurrentWeapon()->GetCurrentFireMode() == EFireMode::Automatic)
	{
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}
}

void APlayerCharacter::FireShot()
{
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon || !Weapon->HasAmmo()) { StopFire(); return; }

	if (UAnimSequence* FireAnim1P = Weapon->GetFireAnim1P()) PlayAnimOnMesh(FirstPersonMesh, FireAnim1P);
	if (UAnimSequence* FireAnim3P = Weapon->GetFireAnim3P()) PlayAnimOnMesh(GetMesh(), FireAnim3P);

	FVector TraceStart;
	FRotator TraceRot;
	GetController()->GetPlayerViewPoint(TraceStart, TraceRot);

	if (RecoilComp)
	{
		FRotator VisualSway = RecoilComp->GetCurrentVisualRecoil().Rotator();
		TraceRot.Yaw += VisualSway.Yaw;
		TraceRot.Pitch += VisualSway.Pitch;
	}

	FVector TraceDir = TraceRot.Vector();
	FVector TraceEnd = TraceStart + (TraceDir * Weapon->WeaponRange);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Weapon);

	bool bDidHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
	if (!bDidHit)
	{
		Hit.TraceEnd = TraceEnd;
		Hit.ImpactPoint = TraceEnd;
	}

	if (IsLocallyControlled() && RecoilComp)
	{
		RecoilComp->Recoil_Fire(Weapon->RecoilData);
	}

	Weapon->PlayFireEffects(Hit);
	Server_FireShot(TraceStart, TraceDir);

	float Delay = 60.0f / Weapon->FireRate;
	if (Weapon->GetCurrentFireMode() == EFireMode::Automatic && bIsTriggerHeld)
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &APlayerCharacter::FireShot, Delay, false);
	else if (Weapon->GetCurrentFireMode() == EFireMode::Burst && BurstShotsRemaining > 0)
	{
		BurstShotsRemaining--;
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &APlayerCharacter::FireShot, Delay, false);
	}
}

bool APlayerCharacter::Server_FireShot_Validate(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir) { return true; }

void APlayerCharacter::Server_FireShot_Implementation(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir)
{
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon) return;

	if (Weapon->ConsumeAmmo(1))
	{
		if (bActionHappening) bActionHappening = false;

		if (UAnimSequence* FireAnim3P = Weapon->GetFireAnim3P())
		{
			Multicast_PlayDynamicMontage(FireAnim3P, 0.05f, 1.0f);
		}

		FVector End = TraceStart + (TraceDir * Weapon->WeaponRange);
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(Weapon);

		bool bDidHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, End, ECC_Visibility, Params);
		if (!bDidHit)
		{
			Hit.TraceEnd = End;
			Hit.ImpactPoint = End;
		}

		Weapon->PlayFireEffects(Hit);

		// --- TRIGGER MINIMAP RED DOT ---
		// (Later, add a check here to see if the weapon has a silencer attachment)
		Multicast_PingMinimap(this, GetActorLocation());

		if (bDidHit)
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				UGameplayStatics::ApplyPointDamage(HitActor, Weapon->WeaponDamage, TraceDir, Hit, GetController(), Weapon, UDamageType::StaticClass());
			}
		}
	}
}

void APlayerCharacter::Reload()
{
	if (bActionHappening) return;
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon || !Weapon->CanReload()) return;

	bActionHappening = true;

	UAnimSequence* Anim1P = (Weapon->HasAmmo()) ? Weapon->GetReloadTacAnim() : Weapon->GetReloadDryAnim();
	PlayAnimOnMesh(FirstPersonMesh, Anim1P);

	UAnimSequence* Anim3P = (Weapon->HasAmmo()) ? Weapon->GetReloadTacAnim3P() : Weapon->GetReloadDryAnim3P();
	PlayAnimOnMesh(GetMesh(), Anim3P);

	Server_Reload();
}

bool APlayerCharacter::Server_Reload_Validate() { return true; }
void APlayerCharacter::Server_Reload_Implementation()
{
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon) return;
	bActionHappening = true;

	UAnimSequence* Anim3P = (Weapon->HasAmmo()) ? Weapon->GetReloadTacAnim3P() : Weapon->GetReloadDryAnim3P();
	Multicast_PlayDynamicMontage(Anim3P, 0.2f, 1.0f);
}

void APlayerCharacter::Inspect()
{
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon || !Weapon->GetInspectAnim()) return;
	bActionHappening = true;

	PlayAnimOnMesh(FirstPersonMesh, Weapon->GetInspectAnim());
	PlayAnimOnMesh(GetMesh(), Weapon->GetInspectAnim3P());

	Server_Inspect();
}

bool APlayerCharacter::Server_Inspect_Validate() { return true; }
void APlayerCharacter::Server_Inspect_Implementation()
{
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon) return;

	bActionHappening = true;

	if (UAnimSequence* Anim3P = Weapon->GetInspectAnim3P())
	{
		Multicast_PlayDynamicMontage(Anim3P, 0.2f, 1.0f);
	}
}

void APlayerCharacter::HandleAnimNotify(ENotifyActionType NotifyType)
{
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon) return;

	switch (NotifyType)
	{
	case ENotifyActionType::AddAmmo:
		if (HasAuthority()) Weapon->ReloadAmmo();
		break;
	case ENotifyActionType::ResetAction:
		ResetActionState();
		break;
	}
}

void APlayerCharacter::CancelAction()
{
	if (UAnimInstance* AnimInst = FirstPersonMesh->GetAnimInstance()) AnimInst->StopAllMontages(0.1f);
	ResetActionState();
}

void APlayerCharacter::ResetActionState() { bActionHappening = false; }

void APlayerCharacter::PlayDynamicMontage(UAnimSequence* AnimSeq, float BlendTime, float PlayRate)
{
	USkeletalMeshComponent* TargetMesh = IsLocallyControlled() ? FirstPersonMesh : GetMesh();
	if (UAnimInstance* AnimInst = TargetMesh->GetAnimInstance())
	{
		AnimInst->PlaySlotAnimationAsDynamicMontage(AnimSeq, FName("DefaultSlot"), BlendTime, BlendTime, PlayRate);
	}
}

void APlayerCharacter::Multicast_PlayDynamicMontage_Implementation(UAnimSequence* AnimSeq, float BlendTime, float PlayRate)
{
	if (!IsLocallyControlled()) PlayAnimOnMesh(GetMesh(), AnimSeq);
}

// =========================================================================
// INVENTORY & UTILS
// =========================================================================

void APlayerCharacter::AttachWeapon(AWeapon* WeaponToAttach, AWeapon* OldWeapon)
{
	if (OldWeapon && !OldWeapon->bIsDropped)
	{
		OldWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		OldWeapon->SetActorHiddenInGame(true);
		OldWeapon->SetActorEnableCollision(false);
		if (OldWeapon->GetFirstPersonMesh()) OldWeapon->GetFirstPersonMesh()->SetCastShadow(false);
	}
	if (WeaponToAttach)
	{
		WeaponToAttach->SetActorHiddenInGame(false);
		WeaponToAttach->SetActorEnableCollision(true);

		SetWeaponVisibility(WeaponToAttach);

		WeaponToAttach->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("WeaponSocket"));
		WeaponToAttach->SetActorRelativeLocation(WeaponToAttach->AttachmentOffset);
		WeaponToAttach->SetActorRelativeRotation(WeaponToAttach->AttachmentRotation);

		if (IsLocallyControlled() && WeaponToAttach->GetFirstPersonMesh())
		{
			USceneComponent* WeaponMesh1P = WeaponToAttach->GetFirstPersonMesh();
			WeaponMesh1P->AttachToComponent(FirstPersonMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("WeaponSocket"));
			WeaponMesh1P->SetRelativeLocation(WeaponToAttach->AttachmentOffset);
			WeaponMesh1P->SetRelativeRotation(WeaponToAttach->AttachmentRotation);

			// Reset the offset so the code knows to calibrate it the first time we aim!
			WeaponToAttach->ADSOffset = FVector::ZeroVector;
		}
	}
}

UAnimSequence* APlayerCharacter::GetCurrentWeaponIdleAnimation() const
{
	if (InventoryComponent && InventoryComponent->GetCurrentWeapon())
	{
		return Cast<UAnimSequence>(InventoryComponent->GetCurrentWeapon()->GetIdleAnimation());
	}
	return nullptr;
}

void APlayerCharacter::ModifyMoney(int32 Amount)
{
	if (HasAuthority()) CurrentMoney = FMath::Max(0, CurrentMoney + Amount);
}

void APlayerCharacter::RequestPurchaseWeapon(TSubclassOf<AWeapon> WeaponToBuy) { Server_PurchaseWeapon(WeaponToBuy); }
bool APlayerCharacter::Server_PurchaseWeapon_Validate(TSubclassOf<AWeapon> WeaponClass) { return WeaponClass != nullptr; }
void APlayerCharacter::Server_PurchaseWeapon_Implementation(TSubclassOf<AWeapon> WeaponClass)
{
	if (InventoryComponent) InventoryComponent->PurchaseWeapon(WeaponClass);
}

void APlayerCharacter::ToggleBuyMenu()
{
	if (!bIsInBuyPhase || !IsLocallyControlled()) return;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	if (BuyMenuInstance && BuyMenuInstance->IsInViewport())
	{
		BuyMenuInstance->RemoveFromParent();
		BuyMenuInstance = nullptr;
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	else if (BuyMenuWidgetClass)
	{
		BuyMenuInstance = CreateWidget<UWeaponBuyMenuWidget>(PC, BuyMenuWidgetClass);
		if (BuyMenuInstance)
		{
			BuyMenuInstance->AddToViewport();
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(BuyMenuInstance->TakeWidget());
			PC->SetInputMode(InputMode);
			PC->SetShowMouseCursor(true);
		}
	}
}

void APlayerCharacter::OnWeaponSwapped()
{
	CancelAction();
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void APlayerCharacter::EquipMelee() { if (InventoryComponent) InventoryComponent->Server_EquipWeapon(EWeaponSlot::Melee); }
void APlayerCharacter::EquipSidearm() { if (InventoryComponent) InventoryComponent->Server_EquipWeapon(EWeaponSlot::Sidearm); }
void APlayerCharacter::EquipPrimary() { if (InventoryComponent) InventoryComponent->Server_EquipWeapon(EWeaponSlot::Primary); }


void APlayerCharacter::SwitchFireMode() { Server_SwitchFireMode(); }
void APlayerCharacter::Server_SwitchFireMode_Implementation()
{
	if (InventoryComponent && InventoryComponent->GetCurrentWeapon()) InventoryComponent->GetCurrentWeapon()->CycleFireMode();
}

FRotator APlayerCharacter::GetAimOffset() const
{
	return (GetControlRotation() - GetActorRotation()).GetNormalized();
}

// --- Helpers ---
void APlayerCharacter::PlayAnimOnMesh(USkeletalMeshComponent* TargetMesh, UAnimSequence* Anim)
{
	if (TargetMesh && Anim)
	{
		if (UAnimInstance* AnimInst = TargetMesh->GetAnimInstance())
		{
			AnimInst->PlaySlotAnimationAsDynamicMontage(Anim, FName("DefaultSlot"), 0.2f, 0.2f, 1.0f);
		}
	}
}

void APlayerCharacter::SetMeshVisibility()
{
	if (GetMesh())
	{
		GetMesh()->SetOwnerNoSee(true);
		GetMesh()->SetCastShadow(true);
		GetMesh()->bCastHiddenShadow = true;
	}

	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetOnlyOwnerSee(true);
		FirstPersonMesh->SetCastShadow(false);
		FirstPersonMesh->bCastHiddenShadow = false;
	}
}

void APlayerCharacter::SetWeaponVisibility(AWeapon* WeaponToAttach)
{
	if (WeaponToAttach->GetFirstPersonMesh())
	{
		// --- THE FIX: Un-hide the 1P mesh when equipping! ---
		WeaponToAttach->GetFirstPersonMesh()->SetHiddenInGame(false);

		WeaponToAttach->GetFirstPersonMesh()->SetCastShadow(false);
		WeaponToAttach->GetFirstPersonMesh()->bCastHiddenShadow = false;
		WeaponToAttach->GetFirstPersonMesh()->SetOnlyOwnerSee(true);
	}

	if (WeaponToAttach->GetThirdPersonMesh())
	{
		WeaponToAttach->GetThirdPersonMesh()->SetCastShadow(true);
		WeaponToAttach->GetThirdPersonMesh()->SetOwnerNoSee(true);
		WeaponToAttach->GetThirdPersonMesh()->bCastHiddenShadow = true;
	}
}

// =========================================================================
// ADS SYSTEM
// =========================================================================

void APlayerCharacter::StartADS()
{
	bIsAiming = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 0.5f;

	// Just-In-Time Calibration!
	if (InventoryComponent)
	{
		if (AWeapon* Weapon = InventoryComponent->GetCurrentWeapon())
		{
			// Only calculate if we haven't done it yet for this weapon
			if (Weapon->ADSOffset.IsZero())
			{
				CalculateWeaponADS(Weapon);
			}
		}
	}
}

void APlayerCharacter::StopADS()
{
	bIsAiming = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacter::Multicast_PingMinimap_Implementation(APlayerCharacter* Shooter, FVector PingLocation)
{
	// 1. If we are the ones shooting, don't show a ping on our own map.
	if (IsLocallyControlled()) return;

	// 2. Grab the LOCAL player's screen/HUD, not the enemy's body!
	if (APlayerController* LocalPC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (APlayerCharacter* LocalCharacter = Cast<APlayerCharacter>(LocalPC->GetPawn()))
		{
			if (LocalCharacter->MasterHUD && LocalCharacter->MasterHUD->Minimap)
			{
				LocalCharacter->MasterHUD->Minimap->SpawnEnemyPing(Shooter, PingLocation);
			}
		}
	}
}

void APlayerCharacter::Multicast_BroadcastKill_Implementation(FKillfeedData KillData)
{
	if (APlayerController* LocalPC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (APlayerCharacter* LocalCharacter = Cast<APlayerCharacter>(LocalPC->GetPawn()))
		{
			// Call your C++ function on the HUD's Killfeed!
			if (LocalCharacter->MasterHUD && LocalCharacter->MasterHUD->Killfeed)
			{
				LocalCharacter->MasterHUD->Killfeed->AddKillMessage(KillData);
			}
		}
	}
}

void APlayerCharacter::CheckForInteractables()
{
	if (!IsLocallyControlled()) return;

	TArray<AActor*> OverlappedWeapons;
	GetCapsuleComponent()->GetOverlappingActors(OverlappedWeapons, AWeapon::StaticClass());

	AWeapon* BestWeapon = nullptr;
	float BestScore = -1.0f;

	FVector CamLoc = CameraComponent->GetComponentLocation();
	FVector CamForward = CameraComponent->GetForwardVector();

	for (AActor* Actor : OverlappedWeapons)
	{
		if (AWeapon* Weapon = Cast<AWeapon>(Actor))
		{
			if (Weapon->bIsDropped)
			{
				FVector DirToWeapon = (Weapon->GetActorLocation() - CamLoc).GetSafeNormal();
				float LookScore = FVector::DotProduct(CamForward, DirToWeapon);

				if (LookScore > 0.0f && LookScore > BestScore)
				{
					BestScore = LookScore;
					BestWeapon = Weapon;
				}
			}
		}
	}

	if (FocusedWeapon != BestWeapon)
	{
		FocusedWeapon = BestWeapon;

		if (MasterHUD && MasterHUD->InteractPrompt)
		{
			if (FocusedWeapon) MasterHUD->InteractPrompt->ShowPrompt(FocusedWeapon);
			else MasterHUD->InteractPrompt->HidePrompt();
		}
	}
}

void APlayerCharacter::Interact()
{
	if (FocusedWeapon)
	{
		Server_Interact(FocusedWeapon);
	}
}

void APlayerCharacter::Server_Interact_Implementation(AWeapon* WeaponToPickup)
{
	if (InventoryComponent)
	{
		InventoryComponent->Server_PickupWeapon(WeaponToPickup);
	}
}
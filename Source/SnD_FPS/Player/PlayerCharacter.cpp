#include "SnD_FPS/Player/PlayerCharacter.h"
#include "SnD_FPS/Weapon/Weapon.h"
#include "SnD_FPS/UI/BuySystem/WeaponBuyMenuWidget.h"
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
#include "DrawDebugHelpers.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- HEALTH & MONEY INIT ---
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	StartingMoney = 800;
	CurrentMoney = StartingMoney;

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));

	// --- MOVEMENT INIT ---
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true; // Better for FPS

	// --- CAMERA INIT ---
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(GetCapsuleComponent());
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->SetRelativeLocation(FVector(-15.0f, 0.0f, 74.0f)); // Your specific offset

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false;
	CameraComponent->FieldOfView = DefaultFOV;

	// --- MESH INIT ---
	GetMesh()->bCastDynamicShadow = true;
	GetMesh()->CastShadow = true;
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.0f, 0.f));
	GetMesh()->SetOwnerNoSee(true);

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(CameraComponent);
	FirstPersonMesh->bCastHiddenShadow = false;
	FirstPersonMesh->CastShadow = false;
	FirstPersonMesh->SetRelativeLocation(FVector(15.f, 0.f, -150.f));
	FirstPersonMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	FirstPersonMesh->SetOnlyOwnerSee(true);
}

void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// --- ALL REPLICATED VARIABLES ---
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
	if (IsLocallyControlled()) FirstPersonMesh->SetCastShadow(true);
	if (HasAuthority()) InventoryComponent->InitializeInventory();

	// Store the default movement values set in Blueprint
	DefaultGroundFriction = GetCharacterMovement()->GroundFriction;
	DefaultBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;

	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// 1P Attachment Fix
	if (IsLocallyControlled() && InventoryComponent && InventoryComponent->GetCurrentWeapon())
	{
		AWeapon* Weap = InventoryComponent->GetCurrentWeapon();
		if (Weap)
		{
			USceneComponent* WeaponRoot = Weap->GetRootComponent();
			if (WeaponRoot && WeaponRoot->GetAttachParent() != FirstPersonMesh)
			{
				WeaponRoot->AttachToComponent(FirstPersonMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("WeaponSocket"));
				WeaponRoot->SetRelativeLocationAndRotation(Weap->AttachmentOffset, Weap->AttachmentRotation);
			}
		}
	}
}

// =========================================================================
// INPUT & MOVEMENT
// =========================================================================

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// ---MOVEMENT ---
		EIC->BindAction(RunAction, ETriggerEvent::Started, this, &APlayerCharacter::StartRunning);
		EIC->BindAction(RunAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopRunning);
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::StartCrouching);
		EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopCrouching);
		EIC->BindAction(SlideAction, ETriggerEvent::Started, this, &APlayerCharacter::StartSliding);
		EIC->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

		// --- NEW COMBAT ---
		EIC->BindAction(FireAction, ETriggerEvent::Started, this, &APlayerCharacter::StartFire);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopFire);
		EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &APlayerCharacter::Reload);
		EIC->BindAction(InspectAction, ETriggerEvent::Started, this, &APlayerCharacter::Inspect);
		EIC->BindAction(FireModeAction, ETriggerEvent::Started, this, &APlayerCharacter::SwitchFireMode);

		// --- UI/INVENTORY ---
		EIC->BindAction(OpenBuyMenuAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleBuyMenu);
		EIC->BindAction(EquipMeleeAction, ETriggerEvent::Started, this, &APlayerCharacter::EquipMelee);
		EIC->BindAction(EquipSidearmAction, ETriggerEvent::Started, this, &APlayerCharacter::EquipSidearm);
		EIC->BindAction(EquipPrimaryAction, ETriggerEvent::Started, this, &APlayerCharacter::EquipPrimary);
	}
}


void APlayerCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D (X = Side/Side, Y = Forward/Back)
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// Get right vector
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement (Y component of input is Forward/Back)
		AddMovementInput(ForwardDirection, MovementVector.Y);

		// Add movement (X component of input is Right/Left)
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y * -1.0f);
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
	// Enforce slow speed LOCALLY
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
	Crouch();
	Server_StartCrouching();
}

void APlayerCharacter::StopCrouching() { UnCrouch(); Server_StopCrouching(); }

void APlayerCharacter::Server_StartCrouching_Implementation()
{
	if (!bIsCrouching)
	{
		// Enforce slow speed ON SERVER
		GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
		bIsCrouching = true;
		Crouch();
	}
}

void APlayerCharacter::Server_StopCrouching_Implementation() { if (bIsCrouching) { bIsCrouching = false; UnCrouch(); } }

// 1. INPUT HANDLER(Client Only)
void APlayerCharacter::StartSliding()
{
	bool bCanSlide = !bIsSliding &&
		GetCharacterMovement()->IsMovingOnGround() &&
		GetCharacterMovement()->Velocity.Size() > (RunSpeed - 50.f);

	if (bCanSlide)
	{
		// A. Apply Locally immediately (Prediction)
		ApplySlidePhysics();

		// B. Tell Server to apply physics too (Fixes Jitter)
		Server_StartSliding();

		// C. Apply Impulse (Velocity)
		FVector SlideDir = GetLastMovementInputVector().GetSafeNormal();
		if (SlideDir.IsNearlyZero()) SlideDir = GetActorForwardVector();
		GetCharacterMovement()->Launch(SlideDir * SlideImpulseAmount);

		// D. Timer
		GetWorldTimerManager().SetTimer(SlideTimerHandle, this, &APlayerCharacter::StopSliding, SlideDuration, false);
	}
}

// 2. PHYSICS HELPER (Shared Logic)
void APlayerCharacter::ApplySlidePhysics()
{
	bIsSliding = true;

	// Make slippery
	GetCharacterMovement()->GroundFriction = SlideFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = SlideBrakingDeceleration;

	// ALLOW FAST MOVEMENT IN CROUCH HITBOX
	GetCharacterMovement()->MaxWalkSpeedCrouched = SlideSpeed;

	Crouch(); // Shrink Hitbox

	// Trigger Camera Shake (Client Only check inside)
	if (IsLocallyControlled() && SlideCameraShakeClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->ClientStartCameraShake(SlideCameraShakeClass, 1.0f);
		}
	}
}

// 3. SERVER RPC
void APlayerCharacter::Server_StartSliding_Implementation()
{
	ApplySlidePhysics();
}

// 4. STOP HANDLER (Client Only - Driven by Timer)
void APlayerCharacter::StopSliding()
{
	if (bIsSliding)
	{
		// A. Reset Locally
		ResetSlidePhysics();

		// B. Reset on Server
		Server_StopSliding();

		GetWorldTimerManager().ClearTimer(SlideTimerHandle);
	}
}

void APlayerCharacter::ResetSlidePhysics()
{
	bIsSliding = false;

	// Restore Grip
	GetCharacterMovement()->GroundFriction = DefaultGroundFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = DefaultBrakingDeceleration;

	// RESTORE SLOW CROUCH SPEED (Crucial Fix)
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;

	// Reset Hitbox
	UnCrouch();

	// Determine Speed based on synced bool
	if (bWantsToRun)
	{
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

// 6. SERVER RPC
void APlayerCharacter::Server_StopSliding_Implementation()
{
	ResetSlidePhysics();
}

void APlayerCharacter::CancelSlide() { if (bIsSliding) StopSliding(); }


// =========================================================================
// HEALTH & DAMAGE (Restored)
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
		OnHealthUpdate();
		return DamageToApply;
	}
	return 0.0f;
}

// =========================================================================
// NEW: COMBAT SYSTEM
// =========================================================================

void APlayerCharacter::StartFire()
{
	bIsTriggerHeld = true;
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon) return;

	// Reload Cancel Check
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
	if (!Weapon || !Weapon->HasAmmo())
	{
		StopFire();
		return;
	}

	// 1. Play Anim
	if (UAnimSequence* FireAnim = Weapon->GetFireAnim1P()) PlayDynamicMontage(FireAnim, 0.05f, 1.0f);

	// --- NEW: Calculate the Hit Point LOCALLY first ---
	FVector TraceStart; FRotator TraceRot;
	GetController()->GetPlayerViewPoint(TraceStart, TraceRot);
	FVector TraceDir = TraceRot.Vector();
	FVector TraceEnd = TraceStart + (TraceDir * Weapon->WeaponRange);

	// Perform a quick trace to find where the crosshair is looking
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Weapon);

	bool bDidHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);
	FVector TargetPoint = bDidHit ? Hit.Location : TraceEnd;
	// --------------------------------------------------

	// 2. Pass the TargetPoint to the Weapon
	Weapon->PlayFireEffects(TargetPoint);

	// 3. Server RPC (Send the same start/dir)
	Server_FireShot(TraceStart, TraceDir);

	// 4. Loop Logic (Keep existing)
	float Delay = 60.0f / Weapon->FireRate;
	if (Weapon->GetCurrentFireMode() == EFireMode::Automatic && bIsTriggerHeld)
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &APlayerCharacter::FireShot, Delay, false);
	else if (Weapon->GetCurrentFireMode() == EFireMode::Burst && BurstShotsRemaining > 0)
	{
		BurstShotsRemaining--;
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &APlayerCharacter::FireShot, Delay, false);
	}
}

bool APlayerCharacter::Server_FireShot_Validate(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir)
{
	return InventoryComponent->GetCurrentWeapon() && InventoryComponent->GetCurrentWeapon()->HasAmmo();
}

void APlayerCharacter::Server_FireShot_Implementation(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir)
{
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon) return;

	if (Weapon->ConsumeAmmo(1))
	{
		if (bActionHappening) bActionHappening = false;

		// 1. Play 3P Anim
		if (UAnimSequence* FireAnim3P = Weapon->GetFireAnim3P())
		{
			Multicast_PlayDynamicMontage(FireAnim3P, 0.05f, 1.0f);
		}

		// 2. HITSCAN LOGIC
		FVector End = TraceStart + (TraceDir * Weapon->WeaponRange);
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(Weapon);

		bool bDidHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, End, ECC_Visibility, Params);

		// 3. DETERMINE TARGET
		FVector TargetPoint = bDidHit ? Hit.Location : End;

		// --- CHANGED: Tell Weapon to fire at the EXACT calculated hit point ---
		// This ensures the Multicast tracer goes to the exact spot the server calculated
		Weapon->PlayFireEffects(TargetPoint);

		// Draw Debug Line (From previous request)
		//DrawDebugLine(GetWorld(), TraceStart, TargetPoint, FColor::Red, false, 3.0f, 0, 1.0f);

		// Apply Damage
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

	UAnimSequence* AnimToPlay = (Weapon->HasAmmo()) ? Weapon->GetReloadTacAnim() : Weapon->GetReloadDryAnim();
	if (!AnimToPlay) return;

	bActionHappening = true;
	PlayDynamicMontage(AnimToPlay, 0.2f, 1.0f);
	Server_Reload();
}

bool APlayerCharacter::Server_Reload_Validate() { return true; }
void APlayerCharacter::Server_Reload_Implementation()
{
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon) return;
	bActionHappening = true;
	UAnimSequence* AnimToPlay = (Weapon->HasAmmo()) ? Weapon->GetReloadTacAnim() : Weapon->GetReloadDryAnim();
	Multicast_PlayDynamicMontage(AnimToPlay, 0.2f, 1.0f);
}

void APlayerCharacter::Inspect()
{
	if (bActionHappening) return;
	AWeapon* Weapon = InventoryComponent->GetCurrentWeapon();
	if (!Weapon || !Weapon->GetInspectAnim()) return;
	bActionHappening = true;
	PlayDynamicMontage(Weapon->GetInspectAnim(), 0.3f, 1.0f);
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
	if (!IsLocallyControlled()) PlayDynamicMontage(AnimSeq, BlendTime, PlayRate);
}

// =========================================================================
// INVENTORY & UTILS
// =========================================================================

void APlayerCharacter::AttachWeapon(AWeapon* WeaponToAttach, AWeapon* OldWeapon)
{
	// Hide/Detach Old
	if (OldWeapon)
	{
		OldWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		OldWeapon->SetActorHiddenInGame(true);
		OldWeapon->SetActorEnableCollision(false);
		if (OldWeapon->GetFirstPersonMesh()) OldWeapon->GetFirstPersonMesh()->SetCastShadow(false);
	}
	// Show/Attach New
	if (WeaponToAttach)
	{
		WeaponToAttach->SetActorHiddenInGame(false);
		WeaponToAttach->SetActorEnableCollision(true);
		if (WeaponToAttach->GetFirstPersonMesh()) WeaponToAttach->GetFirstPersonMesh()->SetCastShadow(true);

		USceneComponent* WeaponRoot = WeaponToAttach->GetRootComponent();
		USkeletalMeshComponent* TargetMesh = IsLocallyControlled() ? FirstPersonMesh : GetMesh();
		WeaponRoot->AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("WeaponSocket"));
		WeaponRoot->SetRelativeLocationAndRotation(WeaponToAttach->AttachmentOffset, WeaponToAttach->AttachmentRotation);
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
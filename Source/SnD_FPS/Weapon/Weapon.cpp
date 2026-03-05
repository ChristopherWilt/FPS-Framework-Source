#include "Weapon.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"

// --- GLOBAL DECAL MANAGER (File Scope) ---
// TWeakObjectPtr prevents crashes if the decal is destroyed by the level changing
static TArray<TWeakObjectPtr<UDecalComponent>> GlobalDecalPool;
static const int32 MaxGlobalDecals = 50; // <--- SET LIMIT HERE

AWeapon::AWeapon()
{
	bReplicates = true;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);


	ThirdPersonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThirdPersonMesh"));
	ThirdPersonMesh->SetupAttachment(RootComponent);
	ThirdPersonMesh->SetOwnerNoSee(true);
	ThirdPersonMesh->bCastDynamicShadow = true;

	FirstPersonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonMesh"));
	FirstPersonMesh->SetupAttachment(RootComponent);
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->bCastHiddenShadow = false;
	FirstPersonMesh->CastShadow = false;

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(ThirdPersonMesh);
	InteractSphere->SetSphereRadius(150.0f); // 1.5 meters!
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (WeaponType == EWeaponType::Melee)
	{
		CurrentMagAmmo = 0; // Or 1, just so UI doesn't look weird
		CurrentReserveAmmo = 0;
	}
	else
	{
		CurrentMagAmmo = MaxMagCapacity;
		CurrentReserveAmmo = StartReserveAmmo;
	}

	if (ADSRotation.IsZero()) ADSRotation = AttachmentRotation;
	if (ADSOffset.IsZero()) ADSOffset = AttachmentOffset;

	// Set Initial Fire Mode
	if (FireConfig.bCanAuto) CurrentFireMode = EFireMode::Automatic;
	else if (FireConfig.bCanSingle) CurrentFireMode = EFireMode::Single;
	else if (FireConfig.bCanBurst) CurrentFireMode = EFireMode::Burst;
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, CurrentMagAmmo);
	DOREPLIFETIME(AWeapon, CurrentReserveAmmo);
	DOREPLIFETIME(AWeapon, CurrentFireMode);
	DOREPLIFETIME(AWeapon, bIsDropped);
}

bool AWeapon::ConsumeAmmo(int32 Amount)
{
	if (WeaponType == EWeaponType::Melee)
	{
		return true;
	}

	if (CurrentMagAmmo >= Amount)
	{
		CurrentMagAmmo -= Amount;
		return true;
	}
	return false;
}

void AWeapon::ReloadAmmo()
{
	int32 Needed = MaxMagCapacity - CurrentMagAmmo;
	int32 ToAdd = FMath::Min(Needed, CurrentReserveAmmo);
	CurrentMagAmmo += ToAdd;
	CurrentReserveAmmo -= ToAdd;
}

bool AWeapon::HasAmmo() const { return CurrentMagAmmo > 0; }
bool AWeapon::IsMagFull() const { return CurrentMagAmmo >= MaxMagCapacity; }
bool AWeapon::CanReload() const 
{
	if (WeaponType == EWeaponType::Melee) return false;

	return CurrentMagAmmo < MaxMagCapacity && CurrentReserveAmmo > 0;
}

void AWeapon::CycleFireMode()
{
	// Simple cycling logic
	switch (CurrentFireMode)
	{
	case EFireMode::Single:
		if (FireConfig.bCanBurst) CurrentFireMode = EFireMode::Burst;
		else if (FireConfig.bCanAuto) CurrentFireMode = EFireMode::Automatic;
		break;
	case EFireMode::Burst:
		if (FireConfig.bCanAuto) CurrentFireMode = EFireMode::Automatic;
		else if (FireConfig.bCanSingle) CurrentFireMode = EFireMode::Single;
		break;
	case EFireMode::Automatic:
		if (FireConfig.bCanSingle) CurrentFireMode = EFireMode::Single;
		else if (FireConfig.bCanBurst) CurrentFireMode = EFireMode::Burst;
		break;
	}
}

void AWeapon::PlayFireEffects(const FHitResult& Hit)
{
	// A. Handle Muzzle & Tracer (Existing Logic)
	if (MuzzleFlashEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlashEffect, FirstPersonMesh, TEXT("MuzzleSocket"), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}

	FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(TEXT("MuzzleSocket"));
	FVector ImpactPoint = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.TraceEnd;

	if (TracerEffect)
	{
		FVector TraceDirection = (ImpactPoint - MuzzleLocation).GetSafeNormal();
		float Distance = (ImpactPoint - MuzzleLocation).Size();

		UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TEXT("User.TraceDirection"), TraceDirection);
			TracerComp->SetFloatParameter(TEXT("User.TracerSpeed"), TracerSpeed);
			TracerComp->SetFloatParameter(TEXT("User.Lifetime"), Distance / TracerSpeed);
		}
	}

	// B. HANDLE IMPACTS
	if (Hit.bBlockingHit)
	{
		// Spawn Particle (Dust/Sparks)
		if (ImpactEffect)
		{
			FRotator ImpactRotation = Hit.ImpactNormal.Rotation();
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, Hit.ImpactPoint, ImpactRotation);
		}

		// Spawn Decal (Bullet Hole)
		if (ImpactDecalMaterial)
		{
			FRotator DecalRotation = Hit.ImpactNormal.Rotation();

			// Spawn the new decal
			UDecalComponent* NewDecal = UGameplayStatics::SpawnDecalAtLocation(
				GetWorld(),
				ImpactDecalMaterial,
				DecalSize,
				Hit.ImpactPoint,
				DecalRotation,
				DecalLifeSpan
			);

			if (NewDecal)
			{
				NewDecal->SetFadeScreenSize(DecalFadeScreenSize);

				// Add to our global list
				GlobalDecalPool.Add(NewDecal);

				// CLEANUP: If we have too many, destroy the oldest one
				if (GlobalDecalPool.Num() > MaxGlobalDecals)
				{
					// Check the oldest item (Index 0)
					if (UDecalComponent* OldDecal = GlobalDecalPool[0].Get())
					{
						OldDecal->DestroyComponent(); // Poof, gone
					}
					GlobalDecalPool.RemoveAt(0); // Remove from list
				}

				// Safety: Remove any null entries (in case they timed out naturally)
				for (int32 i = GlobalDecalPool.Num() - 1; i >= 0; i--)
				{
					if (!GlobalDecalPool[i].IsValid())
					{
						GlobalDecalPool.RemoveAt(i);
					}
				}
			}
		}
	}

	// C. Replicate to others
	if (HasAuthority())
	{
		Multicast_PlayFireFX(Hit);
	}
}

void AWeapon::Multicast_PlayFireFX_Implementation(const FHitResult& Hit)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled()) return; // Skip local player (already played)

	// --- 3P MUZZLE ---
	if (MuzzleFlashEffect && ThirdPersonMesh)
	{
		FVector Muzzle3P = ThirdPersonMesh->GetSocketLocation(TEXT("MuzzleSocket"));
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleFlashEffect, Muzzle3P);

		// --- 3P TRACER ---
		if (TracerEffect)
		{
			FVector ImpactPoint = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.TraceEnd;
			FVector TraceDir = (ImpactPoint - Muzzle3P).GetSafeNormal();
			float Dist = (ImpactPoint - Muzzle3P).Size();

			UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TracerEffect, Muzzle3P);
			if (TracerComp)
			{
				TracerComp->SetVectorParameter(TEXT("User.TraceDirection"), TraceDir);
				TracerComp->SetFloatParameter(TEXT("User.TracerSpeed"), TracerSpeed);
				TracerComp->SetFloatParameter(TEXT("User.Lifetime"), Dist / TracerSpeed);
			}
		}
	}

	// --- IMPACTS (Same as above) ---
	if (Hit.bBlockingHit)
	{
		if (ImpactEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
		}

		if (ImpactDecalMaterial)
		{
			UGameplayStatics::SpawnDecalAtLocation(GetWorld(), ImpactDecalMaterial, DecalSize, Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), DecalLifeSpan);
		}
	}
}

FVector AWeapon::GetActiveSightSocketLocation() const
{
	// 1. If we have a Red Dot or Scope attached, use its SightSocket
	if (AttachedOpticMesh && AttachedOpticMesh->GetStaticMesh())
	{
		if (AttachedOpticMesh->DoesSocketExist(FName("SightSocket")))
		{
			return AttachedOpticMesh->GetSocketLocation(FName("SightSocket"));
		}
	}

	// 2. Fallback: No attachments. Use the Iron Sights socket on the base gun.
	if (FirstPersonMesh && FirstPersonMesh->DoesSocketExist(FName("SightSocket")))
	{
		return FirstPersonMesh->GetSocketLocation(FName("SightSocket"));
	}

	// 3. Absolute Fallback: If you forgot to add a socket, just return the muzzle or center
	UE_LOG(LogTemp, Error, TEXT("NO SIGHT SOCKET FOUND ON WEAPON: %s"), *GetName());
	return FirstPersonMesh->GetComponentLocation();
}

void AWeapon::DropWeapon()
{
	if (HasAuthority())
	{
		bIsDropped = true;
		Multicast_DropWeapon();
	}
}

void AWeapon::Multicast_DropWeapon_Implementation()
{
	// Detach from the player
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);

	// --- FIX 2A: REATTACH 1P MESH TO THE GUN ---
	FirstPersonMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	FirstPersonMesh->SetHiddenInGame(true);

	// Prepare 3P Mesh for Physics
	ThirdPersonMesh->SetOwnerNoSee(false);
	ThirdPersonMesh->SetHiddenInGame(false);

	// --- FIX 2B: FORCE SPHERE TO FOLLOW THE FALLING MESH ---
	InteractSphere->AttachToComponent(ThirdPersonMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Physics and Collision
	ThirdPersonMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ThirdPersonMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ThirdPersonMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ThirdPersonMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	ThirdPersonMesh->SetSimulatePhysics(true);

	// The "Pop and Spin"
	FVector PopImpulse = FVector(FMath::RandRange(-200.f, 200.f), FMath::RandRange(-200.f, 200.f), 400.f);
	ThirdPersonMesh->AddImpulse(PopImpulse, NAME_None, true);
	FVector SpinTorque = FVector(FMath::RandRange(-50.f, 50.f), FMath::RandRange(-50.f, 50.f), FMath::RandRange(-50.f, 50.f));
	ThirdPersonMesh->AddTorqueInDegrees(SpinTorque * 2000.f, NAME_None, true);
}

void AWeapon::Multicast_PickupWeapon_Implementation()
{
	bIsDropped = false;

	// Turn off physics and collision
	ThirdPersonMesh->SetSimulatePhysics(false);
	ThirdPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// --- FIX 3: SNAP THE MESH BACK TO THE ROOT COMPONENT ---
	// Because physics moved the mesh away from the Root, we must reset it 
	// to zero so it perfectly aligns with the weapon socket on the hands!
	ThirdPersonMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	ThirdPersonMesh->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
}
#include "Weapon.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

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

void AWeapon::PlayFireEffects(FVector TraceEnd)
{
	// 1. Local Visuals (1P) - Muzzle Flash
	if (MuzzleFlashEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleFlashEffect, FirstPersonMesh, TEXT("MuzzleSocket"), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}

	// 2. Local Visuals (1P) - Tracer (Converging to Hit Location)
	if (TracerEffect && FirstPersonMesh)
	{
		FVector MuzzleLocation = FirstPersonMesh->GetSocketLocation(TEXT("MuzzleSocket"));

		// --- CHANGED: Calculate direction towards the specific Hit Location ---
		FVector TraceDirection = (TraceEnd - MuzzleLocation).GetSafeNormal();
		float Distance = (TraceEnd - MuzzleLocation).Size();

		UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(TEXT("User.TraceDirection"), TraceDirection);
			TracerComp->SetFloatParameter(TEXT("User.TracerSpeed"), TracerSpeed);
			TracerComp->SetFloatParameter(TEXT("User.Lifetime"), Distance / TracerSpeed);
		}
	}

	// 3. Replication for 3P (Visuals for OTHERS)
	if (HasAuthority())
	{
		FVector Muzzle = ThirdPersonMesh->GetSocketLocation(TEXT("MuzzleSocket"));
		// We pass the SAME TraceEnd so clients see the bullet go to the same spot
		Multicast_PlayFireFX(Muzzle, TraceEnd);
	}
}

void AWeapon::Multicast_PlayFireFX_Implementation(FVector MuzzleLocation, FVector TraceEnd)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled()) return; // Skip local player

	// Play 3P Muzzle Flash
	if (MuzzleFlashEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), MuzzleFlashEffect, MuzzleLocation);
	}

	// Play Tracer
	if (TracerEffect)
	{
		UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp)
		{
			FVector TraceDirection = (TraceEnd - MuzzleLocation).GetSafeNormal();
			float Distance = (TraceEnd - MuzzleLocation).Size();
			TracerComp->SetVectorParameter(TEXT("User.TraceDirection"), TraceDirection);
			TracerComp->SetFloatParameter(TEXT("User.TracerSpeed"), TracerSpeed);
			TracerComp->SetFloatParameter(TEXT("User.Lifetime"), Distance / TracerSpeed);
		}
	}
}
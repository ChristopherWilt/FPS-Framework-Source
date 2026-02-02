// InventoryComponent.cpp

#include "InventoryComponent.h"
#include "SnD_FPS/Weapon/Weapon.h"
#include "SnD_FPS/Player/PlayerCharacter.h" 
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	OwningPlayer = Cast<APlayerCharacter>(GetOwner());
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, MeleeWeapon);
	DOREPLIFETIME(UInventoryComponent, SidearmWeapon);
	DOREPLIFETIME(UInventoryComponent, PrimaryWeapon);
	DOREPLIFETIME(UInventoryComponent, CurrentWeapon);
	DOREPLIFETIME(UInventoryComponent, CurrentSlot);
}

void UInventoryComponent::InitializeInventory()
{
	if (!GetOwner()->HasAuthority()) return;

	// Spawn Melee
	if (DefaultMeleeClass)
	{
		MeleeWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultMeleeClass);
		if (MeleeWeapon)
		{
			MeleeWeapon->SetOwner(GetOwner());
			MeleeWeapon->SetActorHiddenInGame(true);
			MeleeWeapon->SetActorEnableCollision(false);
		}
	}

	// Spawn Sidearm
	if (DefaultSidearmClass)
	{
		SidearmWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultSidearmClass);
		if (SidearmWeapon)
		{
			SidearmWeapon->SetOwner(GetOwner());
			SidearmWeapon->SetActorHiddenInGame(true);
			SidearmWeapon->SetActorEnableCollision(false);
		}
	}

	// Set Initial State
	CurrentWeapon = SidearmWeapon;
	CurrentSlot = EWeaponSlot::Sidearm;

	if (OwningPlayer)
	{
		OwningPlayer->AttachWeapon(CurrentWeapon, nullptr);
	}
}

// -----------------------------------------------------------
// SERVER EQUIP (Update 1 of 2)
// -----------------------------------------------------------
void UInventoryComponent::Server_EquipWeapon_Implementation(EWeaponSlot SlotToEquip)
{
	AWeapon* WeaponToEquip = nullptr;
	switch (SlotToEquip)
	{
	case EWeaponSlot::Melee: WeaponToEquip = MeleeWeapon; break;
	case EWeaponSlot::Sidearm: WeaponToEquip = SidearmWeapon; break;
	case EWeaponSlot::Primary: WeaponToEquip = PrimaryWeapon; break;
	}

	if (WeaponToEquip && WeaponToEquip != CurrentWeapon)
	{
		AWeapon* OldWeapon = CurrentWeapon;
		CurrentWeapon = WeaponToEquip;
		CurrentSlot = SlotToEquip;

		if (OwningPlayer)
		{
			// --- NEW LINE: Reset Gameplay Tags (Cancel Reloads) ---
			OwningPlayer->OnWeaponSwapped();

			// Attach visual meshes
			OwningPlayer->AttachWeapon(CurrentWeapon, OldWeapon);
		}
	}
}

bool UInventoryComponent::PurchaseWeapon(TSubclassOf<AWeapon> WeaponClass)
{
	if (!OwningPlayer || !WeaponClass) return false;

	AWeapon* DefaultWeapon = WeaponClass->GetDefaultObject<AWeapon>();
	if (!DefaultWeapon) return false;

	// Helper: Ensure PlayerCharacter has a "GetMoney" or similar accessor
	if (OwningPlayer->GetCurrentMoney() >= DefaultWeapon->WeaponCost)
	{
		OwningPlayer->ModifyMoney(-DefaultWeapon->WeaponCost);

		EWeaponSlot TargetSlot;
		AWeapon** WeaponToReplace = nullptr;

		switch (DefaultWeapon->WeaponType)
		{
		case EWeaponType::Sidearm:
			TargetSlot = EWeaponSlot::Sidearm;
			WeaponToReplace = &SidearmWeapon;
			break;
		case EWeaponType::SMG:
		case EWeaponType::Shotgun:
		case EWeaponType::Rifle:
		case EWeaponType::Sniper:
			TargetSlot = EWeaponSlot::Primary;
			WeaponToReplace = &PrimaryWeapon;
			break;
		default: return false;
		}

		if (WeaponToReplace && *WeaponToReplace)
		{
			(*WeaponToReplace)->Destroy();
		}

		AWeapon* NewWeapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass);
		if (NewWeapon)
		{
			NewWeapon->SetOwner(GetOwner());
			*WeaponToReplace = NewWeapon;
			Server_EquipWeapon(TargetSlot);
			return true;
		}
	}
	return false;
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PendingAttachmentInfo.bIsPending)
	{
		if (!OwningPlayer) OwningPlayer = Cast<APlayerCharacter>(GetOwner());
		if (!OwningPlayer)
		{
			PendingAttachmentInfo.bIsPending = false;
			return;
		}

		bool bIsReady = false;
		if (OwningPlayer->IsLocallyControlled())
		{
			bIsReady = OwningPlayer->GetPlayerState() != nullptr &&
				PendingAttachmentInfo.WeaponToAttach != nullptr &&
				PendingAttachmentInfo.WeaponToAttach->GetOwner() != nullptr;
		}
		else
		{
			bIsReady = PendingAttachmentInfo.WeaponToAttach != nullptr &&
				PendingAttachmentInfo.WeaponToAttach->GetRootComponent() != nullptr;
		}

		if (bIsReady)
		{
			// Tick doesn't strictly need to call OnWeaponSwapped because
			// OnRep usually triggered the pending state, but we can play it safe:
			// OwningPlayer->OnWeaponSwapped(); // Optional here

			OwningPlayer->AttachWeapon(PendingAttachmentInfo.WeaponToAttach, PendingAttachmentInfo.LastWeapon);
			PendingAttachmentInfo.bIsPending = false;
			PendingAttachmentInfo.WeaponToAttach = nullptr;
			PendingAttachmentInfo.LastWeapon = nullptr;
		}
	}
}

// -----------------------------------------------------------
// CLIENT REPLICATION (Update 2 of 2)
// -----------------------------------------------------------
void UInventoryComponent::OnRep_CurrentWeapon(AWeapon* LastWeapon)
{
	if (!OwningPlayer) OwningPlayer = Cast<APlayerCharacter>(GetOwner());
	if (!OwningPlayer) return;

	if (OwningPlayer->IsLocallyControlled())
	{
		// --- LOCAL PLAYER ---
		bool bIsLocalReady = OwningPlayer->GetPlayerState() != nullptr &&
			CurrentWeapon != nullptr &&
			CurrentWeapon->GetOwner() != nullptr;

		if (bIsLocalReady)
		{
			// --- NEW LINE: Stop Local Animations Immediately ---
			OwningPlayer->OnWeaponSwapped();

			OwningPlayer->AttachWeapon(CurrentWeapon, LastWeapon);
			PendingAttachmentInfo.bIsPending = false;
		}
		else
		{
			PendingAttachmentInfo.WeaponToAttach = CurrentWeapon;
			PendingAttachmentInfo.LastWeapon = LastWeapon;
			PendingAttachmentInfo.bIsPending = true;
		}
	}
	else
	{
		// --- REMOTE PLAYER ---
		bool bIsRemoteReady = CurrentWeapon != nullptr &&
			CurrentWeapon->GetRootComponent() != nullptr;

		if (bIsRemoteReady)
		{
			// Remote players also need to stop animations (e.g. if I see an enemy reloading then they swap)
			OwningPlayer->OnWeaponSwapped();

			OwningPlayer->AttachWeapon(CurrentWeapon, LastWeapon);
			PendingAttachmentInfo.bIsPending = false;
		}
		else
		{
			PendingAttachmentInfo.WeaponToAttach = CurrentWeapon;
			PendingAttachmentInfo.LastWeapon = LastWeapon;
			PendingAttachmentInfo.bIsPending = true;
		}
	}
}
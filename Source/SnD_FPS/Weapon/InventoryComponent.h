// InventoryComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponEnums.h" // Include your new enum file
#include "InventoryComponent.generated.h"

class AWeapon;
class APlayerCharacter;

// Helper struct to store pending attachment info
USTRUCT()
struct FPendingAttachment
{
	GENERATED_BODY()

	UPROPERTY()
	AWeapon* WeaponToAttach = nullptr;

	UPROPERTY()
	AWeapon* LastWeapon = nullptr;

	bool bIsPending = false;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SND_FPS_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** [SERVER] Spawns default weapons and equips one. */
	void InitializeInventory();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** [SERVER] Handles the logic of equipping a weapon from a specific slot. */
	UFUNCTION(Server, Reliable)
	void Server_EquipWeapon(EWeaponSlot SlotToEquip);

	/** [SERVER] Attempts to purchase a weapon. Returns true if successful. */
	bool PurchaseWeapon(TSubclassOf<AWeapon> WeaponClass);

	UFUNCTION(BlueprintPure)
	AWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

protected:
	virtual void BeginPlay() override;

	/** The player character who owns this component. */
	UPROPERTY()
	APlayerCharacter* OwningPlayer;

	/** Stores attachment info if OnRep fires before pawn is ready. */
	UPROPERTY()
	FPendingAttachment PendingAttachmentInfo;

	// --- INVENTORY DATA ---

	/** The player's melee weapon. Always present. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Weapon")
	AWeapon* MeleeWeapon;

	/** The player's sidearm (pistol). Can be replaced. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Weapon")
	AWeapon* SidearmWeapon;

	/** The player's primary weapon (Rifle, SMG, etc.). Can be empty. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Weapon")
	AWeapon* PrimaryWeapon;

	/** A pointer to the currently equipped weapon from our inventory. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentWeapon, Category = "Weapon")
	AWeapon* CurrentWeapon;

	/** The currently equipped weapon slot. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Weapon")
	EWeaponSlot CurrentSlot;

	// --- CONFIGURATION ---

	/** The default melee weapon class to spawn with. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AWeapon> DefaultMeleeClass;

	/** The default sidearm class to spawn with. */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<AWeapon> DefaultSidearmClass;

	/** Called on clients when CurrentWeapon is replicated. */
	UFUNCTION()
	void OnRep_CurrentWeapon(AWeapon* LastWeapon);
};
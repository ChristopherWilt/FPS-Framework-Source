#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "SnD_FPS/Weapon/WeaponEnums.h"
#include "SnD_FPS/Weapon/RecoilComponent.h"
#include "SnD_FPS/Public/Animations/FPSAnimNotify.h"
#include "SnD_FPS/UI/Player/HUDEnums.h"
#include "PlayerCharacter.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class AWeapon;
class USpringArmComponent;
class USkeletalMeshComponent;
class UWeaponBuyMenuWidget;
class UInventoryComponent;
class UAnimSequence;
class UModularPlayerHUD;

UCLASS()
class SND_FPS_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Tick(float DeltaTime) override;

	// --- NEW: WEAPON & COMBAT SYSTEM ---
	void RequestPurchaseWeapon(TSubclassOf<AWeapon> WeaponToBuy);
	void AttachWeapon(AWeapon* WeaponToAttach, AWeapon* OldWeapon);
	void OnWeaponSwapped();

	// Input Handlers
	void StartFire();
	void StopFire();
	void Reload();
	void Inspect();
	void SwitchFireMode();

	// Anim Notify Handler
	void HandleAnimNotify(ENotifyActionType NotifyType);

	// Helpers
	UFUNCTION(BlueprintPure, Category = "Action")
	bool IsActionHappening() const { return bActionHappening; }

	UFUNCTION(BlueprintPure, Category = "Animation")
	UAnimSequence* GetCurrentWeaponIdleAnimation() const;

	void PlayDynamicMontage(UAnimSequence* AnimSeq, float BlendTime = 0.2f, float PlayRate = 1.0f);

	FORCEINLINE USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	FORCEINLINE URecoilComponent* GetRecoilComponent() const { return RecoilComp; }

	// =========================================================================
	// ADS SYSTEM & AUTOMATION
	// =========================================================================
	void StartADS();
	void StopADS();

	/** Runs instantly the moment you equip a weapon to perfectly align the optic */
	void CalculateWeaponADS(AWeapon* Weapon);

protected:
	virtual void BeginPlay() override;

	// =========================================================================
	// PROCEDURAL ARMS ALIGNMENT (The AAA ADS System)
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Procedural Arms")
	FVector DefaultArmsLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Procedural Arms")
	FRotator DefaultArmsRotation;

	FVector CurrentBaseArmsLocation;
	FRotator CurrentBaseArmsRotation;

	// --- SERVER RPCs ---
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PurchaseWeapon(TSubclassOf<AWeapon> WeaponClass);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FireShot(FVector_NetQuantize TraceStart, FVector_NetQuantizeNormal TraceDir);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reload();

	UFUNCTION(Server, Reliable)
	void Server_SwitchFireMode();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayDynamicMontage(UAnimSequence* AnimSeq, float BlendTime, float PlayRate);

	// --- HEALTH SYSTEM ---
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentHealth, Category = "Health")
	float CurrentHealth;

	UFUNCTION()
	void OnRep_CurrentHealth();
	void OnHealthUpdate();

	// --- MESHES ---
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FirstPersonMesh;

	// --- INPUT SYSTEM ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* RunAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SlideAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InspectAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* FireModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* OpenBuyMenuAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* EquipMeleeAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* EquipSidearmAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* EquipPrimaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ADSAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction;

	// --- CAMERA ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCameraComponent* CameraComponent;

	// ------- UI ---------
	
	// Buy Menu
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UWeaponBuyMenuWidget> BuyMenuWidgetClass;

	// --- HUD Hook ---
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UModularPlayerHUD> MasterHUDClass;

	UPROPERTY()
	UModularPlayerHUD* MasterHUD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Settings")
	FCrosshairConfig CurrentCrosshairConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Settings")
	FMinimapConfig CurrentMinimapConfig;

	// --- KILLFEED ---
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastKill(FKillfeedData KillData);

	// --- MINIMAP PINGS ---
	/** Call this when an enemy fires to show a red dot */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PingMinimap(APlayerCharacter* Shooter, FVector PingLocation);



	// --- INVENTORY COMPONENT ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInventoryComponent* InventoryComponent;

public:
	// --- MOVEMENT PROPERTIES ---
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Movement")
	bool bIsSliding = false;

	FTimerHandle SlideTimerHandle;

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsCrouching() const { return bIsCrouching; }

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SlideDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RunSpeed = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SlideSpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RunningFOV = 95;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float DefaultFOV = 90;

	// --- MONEY ---
	UFUNCTION(BlueprintPure, Category = "Player Stats")
	int32 GetCurrentMoney() const { return CurrentMoney; }

	void ModifyMoney(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Animation")
	FRotator GetAimOffset() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	URecoilComponent* RecoilComp;

	void GetisAiming(bool& bAiming) const { bAiming = bIsAiming; }

private:
	// --- MOVEMENT FUNCTIONS ---
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartRunning();
	void StopRunning();
	void StartCrouching();
	void StopCrouching();
	void StartSliding();
	void StopSliding();
	void CancelSlide();

	UFUNCTION(Server, Reliable)
	void Server_StartRunning();
	UFUNCTION(Server, Reliable)
	void Server_StopRunning();

	UFUNCTION(Server, Reliable)
	void Server_StartCrouching();
	UFUNCTION(Server, Reliable)
	void Server_StopCrouching();

	UFUNCTION(Server, Reliable)
	void Server_StartSliding();

	UFUNCTION(Server, Reliable)
	void Server_StopSliding();

	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_Inspect();

	void ApplySlidePhysics();
	void ResetSlidePhysics();

	UPROPERTY(Replicated)
	bool bIsCrouching = false;

	// --- INTERACTION ---
	UPROPERTY()
	AWeapon* FocusedWeapon;

	void CheckForInteractables();
	void Interact();

	UFUNCTION(Server, Reliable)
	void Server_Interact(AWeapon* WeaponToPickup);

	// --- INTERNAL COMBAT LOGIC ---
	void FireShot();
	void CancelAction();
	void ResetActionState();

	bool bIsAiming = false;

	UPROPERTY(Replicated)
	bool bActionHappening = false;

	bool bIsTriggerHeld = false;
	FTimerHandle FireTimerHandle;
	int32 BurstShotsRemaining = 0;

	// --- UI INTERNAL ---
	UPROPERTY()
	UWeaponBuyMenuWidget* BuyMenuInstance;
	void ToggleBuyMenu();

	// --- INPUT HANDLERS ---
	void EquipMelee();
	void EquipSidearm();
	void EquipPrimary();

	bool bIsInBuyPhase = true;
	float DefaultGroundFriction;
	float DefaultBrakingDeceleration;
	bool bWantsToRun = false;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Player Stats")
	int32 StartingMoney;

	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Player Stats")
	int32 CurrentMoney;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CrouchSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Sliding")
	float SlideFriction = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Sliding")
	float SlideBrakingDeceleration = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Sliding")
	float SlideImpulseAmount = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Sliding")
	TSubclassOf<class UCameraShakeBase> SlideCameraShakeClass;

	void SetMeshVisibility();
	void SetWeaponVisibility(AWeapon* WeaponToAttach);
	void PlayAnimOnMesh(USkeletalMeshComponent* TargetMesh, UAnimSequence* Anim);
};
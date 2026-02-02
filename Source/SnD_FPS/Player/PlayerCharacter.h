#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "SnD_FPS/Weapon/WeaponEnums.h" 
#include "Animations/FPSAnimNotify.h" // Include the new Notify system
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

	/** Client-side request to purchase a weapon. */
	void RequestPurchaseWeapon(TSubclassOf<AWeapon> WeaponToBuy);

	/** Attaches weapon meshes. Called on server and clients. */
	void AttachWeapon(AWeapon* WeaponToAttach, AWeapon* OldWeapon);

	/** Call this when inventory swaps weapons to cancel current reloads */
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

	/** Core function to play animations on every bullet */
	void PlayDynamicMontage(UAnimSequence* AnimSeq, float BlendTime = 0.2f, float PlayRate = 1.0f);

	FORCEINLINE USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	FORCEINLINE UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

protected:
	virtual void BeginPlay() override;

	// --- NEW: SERVER RPCs ---
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

	// --- HEALTH SYSTEM (Restored) ---
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
	UInputAction* ReloadAction; // New

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InspectAction; // New

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* FireModeAction; // New

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction; // Logic for WASD

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* OpenBuyMenuAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* EquipMeleeAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* EquipSidearmAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* EquipPrimaryAction;

	// --- CAMERA ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArmComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	UCameraComponent* CameraComponent;

	// --- UI ---
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UWeaponBuyMenuWidget> BuyMenuWidgetClass;

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

	// --- MONEY (Restored) ---
	UFUNCTION(BlueprintPure, Category = "Player Stats")
	int32 GetCurrentMoney() const { return CurrentMoney; }

	void ModifyMoney(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Animation")
	FRotator GetAimOffset() const;

private:
	// --- MOVEMENT FUNCTIONS (Restored) ---
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

	// --- NEW: SERVER SLIDE RPCS ---
	UFUNCTION(Server, Reliable)
	void Server_StartSliding();

	UFUNCTION(Server, Reliable)
	void Server_StopSliding();

	// Helper functions to apply physics settings on both ends
	void ApplySlidePhysics();
	void ResetSlidePhysics();

	UPROPERTY(Replicated)
	bool bIsCrouching = false;

	// --- INTERNAL COMBAT LOGIC (New) ---
	void FireShot();
	void CancelAction();
	void ResetActionState();

	UPROPERTY(Replicated)
	bool bActionHappening = false; // "True" if reloading/inspecting

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


	// Movement Variables
	float DefaultGroundFriction;
	float DefaultBrakingDeceleration;

	// Track if the player wants to run (Shift held) independent of sliding state
	bool bWantsToRun = false;

protected:
	// --- MONEY VARS ---
	UPROPERTY(EditDefaultsOnly, Category = "Player Stats")
	int32 StartingMoney;

	UPROPERTY(VisibleInstanceOnly, Replicated, Category = "Player Stats")
	int32 CurrentMoney;


	// --- CROUCHING ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float CrouchSpeed = 300.f; // Normal slow crouch speed


	// --- SLIDING CONFIGURATION ---

	/** How slippery the ground is while sliding. 0 = Ice. CoD feel is around 0.1 - 0.2 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Sliding")
	float SlideFriction = 0.1f;

	/** How fast you slow down while sliding. Lower = longer slide. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Sliding")
	float SlideBrakingDeceleration = 500.0f; // Standard walking is usually 2048.0f

	/** The boost impulse. MW2019 has a sharp initial burst. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Sliding")
	float SlideImpulseAmount = 900.0f;

	/** The camera shake to play when the slide begins */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Sliding")
	TSubclassOf<class UCameraShakeBase> SlideCameraShakeClass;
};
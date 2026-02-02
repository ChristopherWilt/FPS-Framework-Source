// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "FPSAnimNotify.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ENotifyActionType : uint8
{
	AddAmmo         UMETA(DisplayName = "Add Ammo Point"),
	ResetAction     UMETA(DisplayName = "Action Finished (Unlock)"),
};

UCLASS()
class SND_FPS_API UFPSAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	ENotifyActionType ActionType;
};

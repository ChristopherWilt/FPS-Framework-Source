// Fill out your copyright notice in the Description page of Project Settings.


#include "Animations/FPSAnimNotify.h"
#include "SnD_FPS/Player/PlayerCharacter.h"

void UFPSAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(MeshComp->GetOwner()))
		{
			PlayerChar->HandleAnimNotify(ActionType);
		}
	}
}

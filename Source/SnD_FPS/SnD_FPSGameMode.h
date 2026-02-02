// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SnD_FPSGameMode.generated.h"

UCLASS(minimalapi)
class ASnD_FPSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASnD_FPSGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	void PlayerEliminated(AController* EliminatedController, AController* InstigatorController);

protected:
	virtual void BeginPlay() override;

	void StartRound();
	void EndRound();
	void CheckWinConditions();

	UPROPERTY(EditDefaultsOnly, Category = "Game Rules")
	int32 RoundsToWin;

	UPROPERTY(VisibleInstanceOnly, Category = "Game State")
	int32 AttackerScore;

	UPROPERTY(VisibleInstanceOnly, Category = "Game State")
	int32 DefenderScore;

private:
	TArray<AController*> TeamA;
	TArray<AController*> TeamB;
};
// Copyright Epic Games, Inc. All Rights Reserved.

#include "SnD_FPSGameMode.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerState.h"

ASnD_FPSGameMode::ASnD_FPSGameMode()
{
	RoundsToWin = 5;
	AttackerScore = 0;
	DefenderScore = 0;
}

void ASnD_FPSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Assign players to teams
	if (TeamA.Num() <= TeamB.Num())
	{
		TeamA.Add(NewPlayer);
		// Implement logic to set team in PlayerState
	}
	else
	{
		TeamB.Add(NewPlayer);
	}
}

void ASnD_FPSGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	TeamA.Remove(Exiting);
	TeamB.Remove(Exiting);
}

void ASnD_FPSGameMode::PlayerEliminated(AController* EliminatedController, AController* InstigatorController)
{
	// Handle player death, check for round win conditions
	CheckWinConditions();
}

void ASnD_FPSGameMode::BeginPlay()
{
	Super::BeginPlay();

	// --- ADD THIS DEBUG CODE ---
	if (DefaultPawnClass)
	{
		FString PawnClassName = DefaultPawnClass->GetName();
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("GAME MODE SAYS: Default Pawn Class is: %s"), *PawnClassName));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("GAME MODE SAYS: Default Pawn Class is NOT SET (NULL)!"));
	}
	// --- END OF DEBUG CODE ---

	StartRound();
}

void ASnD_FPSGameMode::StartRound()
{
	// Logic to start a new round, spawn players, etc.
}

void ASnD_FPSGameMode::EndRound()
{
	// Logic to end the round, update scores, and prepare for the next round
}

void ASnD_FPSGameMode::CheckWinConditions()
{
	// Check if a team has been eliminated or if the objective has been completed
}
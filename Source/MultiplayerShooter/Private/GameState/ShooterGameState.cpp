// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/ShooterGameState.h"

#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameMode/ShooterGameMode.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/ShooterPlayerController.h"
#include "PlayerState/ShooterPlayerState.h"

void AShooterGameState::UpdateTopScorePlayerStates(AShooterPlayerState* PlayerState)
{
	if (!PlayerState) return;
	
	if (TopScorePlayerStates.Num() == 0 || TopScore == PlayerState->GetScore())
	{
		HandleTopScorePlayerStates(PlayerState, false);
		HandleTopScore(PlayerState->GetScore());
	}
	else if (TopScore < PlayerState->GetScore())
	{
		HandleTopScorePlayerStates(PlayerState, true);
		HandleTopScore(PlayerState->GetScore());
	}
}

void AShooterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterGameState, TopScore);
	DOREPLIFETIME(AShooterGameState, TopScorePlayerStates);
}


void AShooterGameState::HandleTopScore(float Score)
{
	Server_HandleTopScore(Score);
}

void AShooterGameState::Server_HandleTopScore_Implementation(float Score)
{
	TopScore = Score;
}

void AShooterGameState::OnRep_HandleTopScore(float Score)
{
	AShooterPlayerController* ShooterPlayerController = Cast<AShooterPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (ShooterPlayerController && ShooterPlayerController->IsLocalController())
	{
		// Updating the TopScore in the HUD
		ShooterPlayerController->UpdateTopScore();
	}
}

void AShooterGameState::HandleTopScorePlayerStates(AShooterPlayerState* PlayerState, bool bRewrite)
{
	Server_HandleTopScorePlayerStates(PlayerState, bRewrite);
}

void AShooterGameState::Server_HandleTopScorePlayerStates_Implementation(AShooterPlayerState* PlayerState, bool bRewrite)
{
	if (bRewrite)
	{
		TopScorePlayerStates.Empty();
	}
	TopScorePlayerStates.AddUnique(PlayerState);
}

void AShooterGameState::OnRep_HandleTopScorePlayerStates(TArray<class AShooterPlayerState*> TopScorePS)
{
	AShooterPlayerController* ShooterPlayerController = Cast<AShooterPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (ShooterPlayerController && ShooterPlayerController->IsLocalController())
	{
		// Updating the TopScorePlayer in the HUD
		ShooterPlayerController->UpdateTopScorePlayer();
	}
}

void AShooterGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();

	if (!GetWorld())
	{
		return;
	}

	OnGameStateChanged.Broadcast(GetMatchState());

}

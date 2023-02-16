// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/ShooterPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/ShooterPlayerController.h"

void AShooterPlayerState::UpdateScore()
{
	SetScore(GetScore() + ScoreAmount);

	ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(GetOwningController());
	if (!ShooterPlayerController) return;
	
	ShooterPlayerController->UpdatePlayerScore(GetScore());
}

void AShooterPlayerState::UpdateDefeats()
{
	Defeats += 1;

	ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(GetOwningController());
	if (!ShooterPlayerController) return;
	
	ShooterPlayerController->UpdatePlayerDefeats(Defeats);
}


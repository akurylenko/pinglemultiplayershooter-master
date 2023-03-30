// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInstance/MultiShooterGameInstance.h"
#include "GameMode/ShooterGameMode.h"
#include "PlayerController/ShooterPlayerController.h"
#include "PlayerState/ShooterPlayerState.h"
#include "GameState/ShooterGameState.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UMultiShooterGameInstance::SaveCurrentLevel()
{
	CurrentSaveGame->SavedPlayers.Empty();

	if (!GetWorld())
	{
		return;
	}

	AShooterGameState* GameState = Cast<AShooterGameState>(GetWorld()->GetGameState());
	if (!GameState)
	{
		return;
	}

	AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PlayerController)
	{
		return;
	}
		AShooterPlayerState* CurPlayerState = PlayerController->GetPlayerState<AShooterPlayerState>();

	for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
	{
		AShooterPlayerState* PlayerState = Cast<AShooterPlayerState>(GameState->PlayerArray[i]);;
		if (CurPlayerState == PlayerState)
		{
			PlayerState->SavePlayerState(CurrentSaveGame);
			break; // single player only at this point
		}
	}

}

float UMultiShooterGameInstance::LoadScore()
{
	/*if (SaveLevelData && SaveLevelData->MLevelSaveInfo.Num() > 0)
	{
		return SaveLevelData->MLevelSaveInfo["LevelSave"].PlayerScore;
	}*/

	return 0.0f;
}

int32 UMultiShooterGameInstance::LoadMapsNum()
{
	/*if (!SaveLevelData)
	{
		return 0;
	}

	AShooterPlayerController* PlayerController = Cast<AShooterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (PlayerController)
	{
		AShooterPlayerState* PlayerState = PlayerController->GetPlayerState<AShooterPlayerState>();

		if (PlayerState)
		{
			PlayerState->SetScore(LoadScore());
		}
	}

	if (SaveLevelData->MLevelSaveInfo.Num() > 0)
	{
		return SaveLevelData->MLevelSaveInfo["LevelSave"].MapsNum;
	}*/

	return 0;
}

void UMultiShooterGameInstance::RemoveLevelData()
{
	/*if (SaveLevelData && SaveLevelData->MLevelSaveInfo.Num() > 0)
	{
		SaveLevelData->MLevelSaveInfo.Remove("LevelSave");
	}*/
}

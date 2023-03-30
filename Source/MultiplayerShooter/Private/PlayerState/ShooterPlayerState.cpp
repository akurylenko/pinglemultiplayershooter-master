// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/ShooterPlayerState.h"
#include "SaveGame/MultiShooterSaveGame.h"
#include "GameMode/ShooterGameMode.h"
#include "SaveGame/SaveGameSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CoreDelegates.h"
#include "PlayerController/ShooterPlayerController.h"

void AShooterPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	FCoreDelegates::OnExit.AddUObject(this, &AShooterPlayerState::OnGameClosed);
}

void AShooterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerState, Defeats);
}

void AShooterPlayerState::UpdateScore()
{
	SetScore(GetScore() + ScoreAmount);

	ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(GetOwningController());
	if (!ShooterPlayerController) return;
	
	ShooterPlayerController->UpdatePlayerScore(GetScore());
}

void AShooterPlayerState::UpdateDefeats()
{
	SetDefeats(Defeats + 1);
}

void AShooterPlayerState::OnRep_SetDefeats(int32 PrevDefeats)
{
	ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(GetOwningController());
	if (!ShooterPlayerController) return;

	ShooterPlayerController->UpdatePlayerDefeats(Defeats);
}

bool AShooterPlayerState::Server_SetDefeats_Validate(int32 NewDefeats)
{
	return true;
}

void AShooterPlayerState::Server_SetDefeats_Implementation(int32 NewDefeats)
{
	int32 PrevDefeats = Defeats;
	Defeats = NewDefeats;

	OnRep_SetDefeats(PrevDefeats);
}

void AShooterPlayerState::SetDefeats(int32 NewDefeats)
{
	Server_SetDefeats(NewDefeats);
}


void AShooterPlayerState::FreePlayerState_Implementation(UMultiShooterSaveGame* SaveObject)
{
	if (SaveObject)
	{
		SetScore(0.0);
		SetDefeats(0);
		// Gather all relevant data for player
		FPlayerSaveData SaveData;

		SaveData.PlayerScore = GetScore();
		SaveData.PlayerDefeats = Defeats;
		// Stored as FString for simplicity (original Steam ID is uint64)
		SaveData.PlayerID = GetUniqueId().ToString();
		SaveData.ActorName = GetPlayerName();

		SaveObject->SavedPlayers.Add(SaveData);
	}
}

void AShooterPlayerState::OnGameClosed()
{
	if (!GetWorld())
	{
		return;
	}
	USaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();

	AShooterGameMode* GameMode = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode());
	if (!GameMode || !SG)
	{
		return;
	}
	// Optional slot name (Falls back to slot specified in SaveGameSettings class/INI otherwise)
	FString SelectedSaveSlot = UGameplayStatics::ParseOption(GameMode->OptionsString, "SaveGame");
	SG->RemoveSaveGame(this);
}

void AShooterPlayerState::SavePlayerState_Implementation(UMultiShooterSaveGame* SaveObject)
{
	if (SaveObject)
	{
		// Gather all relevant data for player
		FPlayerSaveData SaveData;

		SaveData.PlayerScore = GetScore();
		SaveData.PlayerDefeats = Defeats;
		// Stored as FString for simplicity (original Steam ID is uint64)
		SaveData.PlayerID = GetUniqueId().ToString();
		SaveData.ActorName = GetPlayerName();


		SaveObject->SavedPlayers.Add(SaveData);
	}
	FCoreDelegates::OnExit.Clear();
}

void AShooterPlayerState::LoadPlayerState_Implementation(UMultiShooterSaveGame* SaveObject)
{
	if (SaveObject)
	{
		FPlayerSaveData* FoundData = SaveObject->GetPlayerData(this);
		if (FoundData)
		{
			//Credits = SaveObject->Credits;
			// Makes sure we trigger credits changed event
			SetScore(FoundData->PlayerScore);
			SetDefeats(FoundData->PlayerDefeats);

			/*ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(GetOwningController());
			if (!ShooterPlayerController) return;

			ShooterPlayerController->UpdatePlayerScore(GetScore());
			ShooterPlayerController->UpdatePlayerDefeats(Defeats);*/
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not find SaveGame data for player id '%i'."), GetPlayerId());
		}
	}
	FCoreDelegates::OnExit.AddUObject(this, &AShooterPlayerState::OnGameClosed);
}

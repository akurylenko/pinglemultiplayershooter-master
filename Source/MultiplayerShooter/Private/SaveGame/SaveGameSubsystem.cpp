// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGame/SaveGameSubsystem.h"
#include "SaveGame/SaveGameSettings.h"
#include "PlayerState/ShooterPlayerState.h"
#include "SaveGame/MultiShooterSaveGame.h"
#include "GameState/ShooterGameState.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

void USaveGameSubsystem::HandleStartingNewPlayer(AController* NewPlayer)
{
	AShooterPlayerState* PlayerState = NewPlayer->GetPlayerState<AShooterPlayerState>();
	if (ensure(PlayerState))
	{
		PlayerState->LoadPlayerState(CurrentSaveGame);
	}
}

void USaveGameSubsystem::SetSlotName(FString NewSlotName)
{
	// Ignore empty name
	if (NewSlotName.Len() == 0)
	{
		return;
	}

	CurrentSlotName = NewSlotName;
}

void USaveGameSubsystem::WriteSaveGame()
{
	if (CurrentSaveGame)
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

	for (TObjectPtr<APlayerState> ShooterPS : GameState->PlayerArray)
	{
		AShooterPlayerState* PlayerState = Cast<AShooterPlayerState>(ShooterPS);;
		if (PlayerState)
		{
			PlayerState->SavePlayerState(CurrentSaveGame);
			//break; // single player only at this point
		}
	}

	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, CurrentSlotName, 0);

	OnSaveGameWritten.Broadcast(CurrentSaveGame);
}

void USaveGameSubsystem::LoadSaveGame(FString InSlotName)
{
	// Update slot name first if specified, otherwise keeps default name
	SetSlotName(InSlotName);

	if (UGameplayStatics::DoesSaveGameExist(CurrentSlotName, 0))
	{
		CurrentSaveGame = Cast<UMultiShooterSaveGame>(UGameplayStatics::LoadGameFromSlot(CurrentSlotName, 0));
		if (CurrentSaveGame == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load SaveGame Data."));
			return;
		}

		if (!GetWorld())
		{
			return;
		}

		AShooterGameState* GameState = Cast<AShooterGameState>(GetWorld()->GetGameState());
		if (!GameState)
		{
			return;
		}

		for (TObjectPtr<APlayerState> ShooterPS : GameState->PlayerArray)
		{
			AShooterPlayerState* PlayerState = Cast<AShooterPlayerState>(ShooterPS);;
			if (PlayerState)
			{
				PlayerState->LoadPlayerState(CurrentSaveGame);
			//	break; // single player only at this point
			}
		}

		OnSaveGameLoaded.Broadcast(CurrentSaveGame);
	}
	else
	{
		CurrentSaveGame = Cast<UMultiShooterSaveGame>(UGameplayStatics::CreateSaveGameObject(UMultiShooterSaveGame::StaticClass()));

		UE_LOG(LogTemp, Log, TEXT("Created New SaveGame Data."));
	}
}

void USaveGameSubsystem::RemoveSaveGame(AShooterPlayerState* CurPlayerState)
{
	if (CurrentSaveGame)
		CurrentSaveGame->SavedPlayers.Empty();
	else
	{
		CurrentSaveGame = Cast<UMultiShooterSaveGame>(UGameplayStatics::LoadGameFromSlot(CurrentSlotName, 0));
		if (CurrentSaveGame == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load SaveGame Data."));
			return;
		}
		CurrentSaveGame->SavedPlayers.Empty();
	}

	if (!GetWorld() || !CurPlayerState)
	{
		return;
	}

	AShooterGameState* GameState = Cast<AShooterGameState>(GetWorld()->GetGameState());
	if (!GameState)
	{
		return;
	}

	for (TObjectPtr<APlayerState> ShooterPS : GameState->PlayerArray)
	{
		AShooterPlayerState* PlayerState = Cast<AShooterPlayerState>(ShooterPS);
		if (PlayerState)
		{
			PlayerState->FreePlayerState(CurrentSaveGame);
			//break; // single player only at this point
		}
	}

	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, CurrentSlotName, 0);

	OnSaveGameWritten.Broadcast(CurrentSaveGame);
}

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const USaveGameSettings* SGSettings = GetDefault<USaveGameSettings>();
	// Access defaults from DefaultGame.ini
	CurrentSlotName = SGSettings->SaveSlotName;

	// Make sure it's loaded into memory .Get() only resolves if already loaded previously elsewhere in code
	UDataTable* DummyTable = SGSettings->DummyTablePath.LoadSynchronous();
	//DummyTable->GetAllRows() // We don't need this table for anything, just an content reference example
}

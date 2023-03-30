// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/ShooterGameMode.h"
#include "Character/MainCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "GameState/ShooterGameState.h"
#include "GameInstance/MultiShooterGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerController/ShooterPlayerController.h"
#include "PlayerState/ShooterPlayerState.h"
#include "SaveGame/SaveGameSubsystem.h"
#include "Misc/CoreDelegates.h"


namespace MatchState
{
	const FName Cooldown = FName(TEXT("Cooldown"));
}

AShooterGameMode::AShooterGameMode()
{
	bDelayedStart = true;	
}

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	LevelStartingTime = GetWorld()->GetTimeSeconds();

	UMultiShooterGameInstance* GameInstance = Cast<UMultiShooterGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		CurrentMapNum = GameInstance->LoadMapsNum();
	}
}

void AShooterGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f) StartMatch();
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f) SetMatchState(MatchState::Cooldown);
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = WarmupTime + MatchTime + CooldownTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f) RestartGame();
	}
}

void AShooterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShooterPlayerController* ShooterPlayerController = Cast<AShooterPlayerController>(*It))
		{
			ShooterPlayerController->OnMatchStateSet(MatchState);
		}
	}
}

void AShooterGameMode::SaveGame()
{
	USaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	// Immediately auto save on death
	SG->WriteSaveGame();
}

void AShooterGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// (Save/Load logic moved into new SaveGameSubsystem)
	USaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();

	// Optional slot name (Falls back to slot specified in SaveGameSettings class/INI otherwise)
	FString SelectedSaveSlot = UGameplayStatics::ParseOption(Options, "SaveGame");
	SG->LoadSaveGame(SelectedSaveSlot);
}

void AShooterGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Calling Before Super:: so we set variables before 'USaveGameSubsystem' is called in PlayerController (which is where we instantiate UI)
	USaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	SG->HandleStartingNewPlayer(NewPlayer);

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void AShooterGameMode::ChangeMap()
{
	UWorld* World = GetWorld();
	if (World && CurrentMapNum < MapNames.Num() - 1)
	{
		bUseSeamlessTravel = true;

		CurrentMapNum++;

		SaveGame();

		if (MapNames.IsEmpty())
		{
			World->ServerTravel(FString("/Game/Maps/StartupMap?listen"));
		}
		else if (MapNames.Num() > 1)
		{
			FString MapName = MapNames[FMath::RandRange(0, MapNames.Num() - 1)];
			
			FString CurrentLevelName = GetWorld()->GetMapName();
			CurrentLevelName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

			while (MapName == CurrentLevelName)
			{
				MapName = MapNames[FMath::RandRange(0, MapNames.Num() - 1)];
			}


			World->ServerTravel(FString("/Game/Maps/" + MapName + "?listen"));
		}
		else
		{
			FString MapName = MapNames[0];

			World->ServerTravel(FString("/Game/Maps/" + MapName + "?listen"));
		}
	}
}

void AShooterGameMode::PlayerEliminated(AMainCharacter* EliminatedCharacter, AShooterPlayerController* VictimController, AShooterPlayerController* AttackerController)
{
	if (!EliminatedCharacter || !AttackerController || !VictimController) return;

	AShooterPlayerState* AttackerPlayerState = AttackerController->GetPlayerState<AShooterPlayerState>();
	AShooterPlayerState* VictimPlayerState = VictimController->GetPlayerState<AShooterPlayerState>();
	if (!AttackerPlayerState || !VictimPlayerState) return;

	// Need to check if it's suicide.
	if (AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->UpdateScore();
		VictimPlayerState->UpdateDefeats();
	}
	EliminatedCharacter->Eliminated();

	AShooterGameState* ShooterGameState = GetGameState<AShooterGameState>();
	if (!ShooterGameState) return;

	// Update GameState Info
	ShooterGameState->UpdateTopScorePlayerStates(AttackerPlayerState);
}

void AShooterGameMode::RequestRespawn(AMainCharacter* EliminatedCharacter, AController* EliminatedController)
{
	// Detach character from the controller and destroy.
	if (!EliminatedCharacter) return;
	EliminatedCharacter->Reset();
	EliminatedCharacter->Destroy();

	// Respawn a new character with a random reborn-spot for the controller.
	if (!EliminatedController) return;
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	const int32 PlayerStartIndex = FMath::RandRange(0, PlayerStarts.Num() - 1);

	RestartPlayerAtPlayerStart(EliminatedController, PlayerStarts[PlayerStartIndex]);
}


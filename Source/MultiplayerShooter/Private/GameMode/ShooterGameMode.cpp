// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/ShooterGameMode.h"
#include "Character/MainCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "GameState/ShooterGameState.h"
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

	GetAllMapNames();

	CurrentMapNum = 0;
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


void AShooterGameMode::StartPlay()
{
	Super::StartPlay();
	// (Save/Load logic moved into new SaveGameSubsystem)
	USaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();

	// Optional slot name (Falls back to slot specified in SaveGameSettings class/INI otherwise)
	FString SelectedSaveSlot = UGameplayStatics::ParseOption(OptionsString, "SaveGame");
	SG->LoadSaveGame(SelectedSaveSlot);
}

void AShooterGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void AShooterGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Calling Before Super:: so we set variables before 'USaveGameSubsystem' is called in PlayerController (which is where we instantiate UI)
	USaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	SG->HandleStartingNewPlayer(NewPlayer);

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void AShooterGameMode::GetAllMapNames()
{
	TArray<FString> temp;

	IFileManager::Get().FindFilesRecursive(MapNames, *FPaths::ProjectContentDir(), TEXT("*.umap"), true, false, false);

	for (int i = 0; i < MapNames.Num(); ++i) {
		MapNames[i].ParseIntoArray(temp, TEXT("/"), true);
		MapNames[i] = temp[temp.Num() - 1];
		MapNames[i].ParseIntoArray(temp, TEXT("."), true);
		MapNames[i] = temp[0];
	}
}

void AShooterGameMode::ChangeMap()
{
	UWorld* World = GetWorld();
	if (World && CurrentMapNum < MapNames.Num() - 1)
	{
		bUseSeamlessTravel = true;

		CurrentMapNum++;

		SaveGame();

		FString MapName = MapNames[FMath::RandRange(0, MapNames.Num() - 1)];

		FString CurrentLevelName = GetWorld()->GetMapName();
		CurrentLevelName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

		int i = 0;
		while (MapNames.Num() > 1 && (MapName == CurrentLevelName || MapName.Contains("_NoForPlay") || MapName.Contains("Lobby")) && i < MapNames.Num())
		{
			MapName = MapNames[FMath::RandRange(0, MapNames.Num() - 1)];
			++i;
		}

		World->ServerTravel(FString(MapName + "?listen"));
	}
}

void AShooterGameMode::PlayerEliminated(AMainCharacter* EliminatedCharacter, AShooterPlayerController* VictimController, AShooterPlayerController* AttackerController)
{
	if (!EliminatedCharacter || !AttackerController || !VictimController) return;

	AShooterPlayerState* AttackerPlayerState = AttackerController->GetPlayerState<AShooterPlayerState>();
	AShooterPlayerState* VictimPlayerState = VictimController->GetPlayerState<AShooterPlayerState>();
	if (!AttackerPlayerState || !VictimPlayerState)
	{
		return;
	}

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


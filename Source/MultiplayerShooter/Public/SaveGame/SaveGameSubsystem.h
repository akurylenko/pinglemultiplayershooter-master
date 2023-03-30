// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGameSubsystem.generated.h"

class UMultiShooterSaveGame;
class AShooterPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameSignature, class UMultiShooterSaveGame*, SaveObject);

/**
 * Handles all saving/loading of game state and player data
 */
UCLASS(meta = (DisplayName = "SaveGame System"))
class MULTIPLAYERSHOOTER_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:

	/* Name of slot to save/load to disk. Filled by SaveGameSettings (can be overriden from GameMode's InitGame()) */
	FString CurrentSlotName;

	UPROPERTY()
	TObjectPtr<UMultiShooterSaveGame> CurrentSaveGame;

public:

	/* Restore serialized data from PlayerState into player */
	void HandleStartingNewPlayer(AController* NewPlayer);


	/* Change slot name, will be used next time we load/save data */
	UFUNCTION(BlueprintCallable)
	void SetSlotName(FString NewSlotName);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void WriteSaveGame();

	/* Load from disk, optional slot name */
	void LoadSaveGame(FString InSlotName = "");

	void RemoveSaveGame(AShooterPlayerState* CurPlayerState);

	UPROPERTY(BlueprintAssignable)
	FOnSaveGameSignature OnSaveGameLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnSaveGameSignature OnSaveGameWritten;

	/* Initialize Subsystem, good moment to load in SaveGameSettings variables */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};

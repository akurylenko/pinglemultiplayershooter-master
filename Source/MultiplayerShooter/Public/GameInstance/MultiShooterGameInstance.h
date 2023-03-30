// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SaveGame/MultiShooterSaveGame.h"
#include "MultiShooterGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSHOOTER_API UMultiShooterGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	void SaveCurrentLevel();

	float LoadScore();

	int32 LoadMapsNum();

	void RemoveLevelData();
private:
	/* Name of slot to save/load to disk. Filled by SaveGameSettings (can be overriden from GameMode's InitGame()) */
	FString CurrentSlotName;

	UPROPERTY()
	TObjectPtr<UMultiShooterSaveGame> CurrentSaveGame;
};

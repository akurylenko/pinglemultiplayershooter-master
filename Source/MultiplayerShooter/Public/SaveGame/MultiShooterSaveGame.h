// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MultiShooterSaveGame.generated.h"

class AShooterPlayerState;

USTRUCT(BlueprintType)
struct FPlayerSaveData
{
	GENERATED_BODY()

public:

	/* Player Id defined by the online sub system (such as Steam) converted to FString for simplicity  */
	UPROPERTY()
	FString PlayerID;

	UPROPERTY()
	FString ActorName;

	UPROPERTY()
	float PlayerScore;

	UPROPERTY()
	int32 PlayerDefeats;

	/* Contains all 'SaveGame' marked variables of the Actor */
	UPROPERTY()
	TArray<uint8> ByteData;
};


UCLASS()
class MULTIPLAYERSHOOTER_API UMultiShooterSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FPlayerSaveData> SavedPlayers;

	FPlayerSaveData* GetPlayerData(AShooterPlayerState* PlayerState);

	UPROPERTY()
	int32 MapsNum;
};

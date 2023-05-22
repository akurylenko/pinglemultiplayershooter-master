// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/LobbyGameMode.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "DedicatedGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSHOOTER_API ADedicatedGameMode : public ALobbyGameMode
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;

	int32 NumPublicConnections{ 4 };
	FString MatchType{ TEXT("FreeForAll") };
	FString LobbyPath{ "/Game/Maps/Lobby" };
	FString SessionName{ "" };

private:
	UFUNCTION()
	void HandleOnCreatedSession(bool bWasSuccessful);
};

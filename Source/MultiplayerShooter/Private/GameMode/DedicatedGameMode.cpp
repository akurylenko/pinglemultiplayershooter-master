// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/DedicatedGameMode.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "FindSessionsCallbackProxy.h"
#include "OnlineSubsystem.h"
#include "MultiplayerSessionsSubsystem.h"


void ADedicatedGameMode::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ADedicatedGameMode::HandleOnCreatedSession);
			MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType, true);
		}
	}
}

void ADedicatedGameMode::HandleOnCreatedSession(bool bWasSuccessful)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem)
		{

			MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.RemoveDynamic(this, &ADedicatedGameMode::HandleOnCreatedSession);
			MultiplayerSessionsSubsystem->StartSession();
		}
	}
}
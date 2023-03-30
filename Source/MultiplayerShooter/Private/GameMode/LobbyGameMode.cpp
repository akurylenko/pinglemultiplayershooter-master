// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"
#include "GameFramework/GameState.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	if (NumberOfPlayers == 2)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			bUseSeamlessTravel = true;
			if (!MapNames.IsEmpty())
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Cyan,
					FString::Printf(TEXT("/Game/Maps/StartupMap"))
				);

				World->ServerTravel(FString("/Game/Maps/StartupMap?listen"));

			}
			else if (MapNames.Num() > 1)
			{
				FString MapName = MapNames[FMath::RandRange(0, MapNames.Num() - 1)];

				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Cyan,
					MapName
				);

				World->ServerTravel(FString("/Game/Maps/" + MapName + "?listen"));
			}
			else
			{
				FString MapName = MapNames[0];

				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Cyan,
					MapName
				);

				World->ServerTravel(FString("/Game/Maps/" + MapName + "?listen"));
			}
		}
	}
}

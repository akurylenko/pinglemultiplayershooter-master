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

			GetAllMapNames();

			FString MapName = MapNames[FMath::RandRange(0, MapNames.Num() - 1)];

			FString CurrentLevelName = GetWorld()->GetMapName();
			CurrentLevelName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

			while (MapNames.Num() > 1 && (MapName == CurrentLevelName || MapName.Contains("_NoForPlay")))
			{
				MapName = MapNames[FMath::RandRange(0, MapNames.Num() - 1)];
			}

			World->ServerTravel(FString(MapName + "?listen"));
		}
	}
}

void ALobbyGameMode::GetAllMapNames()
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

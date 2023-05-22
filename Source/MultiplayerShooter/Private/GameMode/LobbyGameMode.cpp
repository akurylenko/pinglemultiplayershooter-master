// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/LobbyGameMode.h"
#include "GameFramework/GameState.h"


void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	GEngine->AddOnScreenDebugMessage(
		-1,
		15.f,
		FColor::Cyan,
		FString::Printf(TEXT("PostLogin, NumberOfPlayers: %f"), NumberOfPlayers)
	);
	
	if (NumberOfPlayers > 1)
	{

		UWorld* World = GetWorld();
		if (World)
		{
			bUseSeamlessTravel = true;

			if (GetAllMapNames())
			{
				FString MapName = MapNames[0];

				bool bHaveLevelMap = false;
				FString CurrentLevelName = GetWorld()->GetMapName();

				if (MapNames.Num() > 1)
				{
					MapName = MapNames[FMath::RandRange(0, MapNames.Num() - 1)];
					for (FString MapTest : MapNames)
					{
						if (MapTest != CurrentLevelName && !MapTest.Contains("_NoForPlay") && !MapTest.Contains("Lobby") && !MapTest.Contains("Entry"))
						{
							bHaveLevelMap = true;
							break;
						}
					}
				}


				CurrentLevelName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

				while (bHaveLevelMap && MapNames.Num() > 1 && (MapName == CurrentLevelName || MapName.Contains("_NoForPlay") || MapName.Contains("Lobby")))
				{
					MapName = MapNames[FMath::RandRange(0, MapNames.Num() - 1)];
				}

				World->ServerTravel(FString(MapName + "?listen"));
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Red,
					FString::Printf(TEXT("PostLogin, No Map"))
				);
			}
		}
	}
}

void ALobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Red,
			FString::Printf(TEXT("PreLogin, 1 You in"))
		);
	}

	Super::PreLogin(Options, Address,UniqueId, ErrorMessage);
}

APlayerController* ALobbyGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Red,
			FString::Printf(TEXT("Login, 2 You in"))
		);
	}
	
	return Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);
}

bool ALobbyGameMode::GetAllMapNames()
{
	TArray<FString> temp;

	IFileManager::Get().FindFilesRecursive(MapNames, *FPaths::ProjectContentDir(), TEXT("*.umap"), true, false, false);

	if (MapNames.IsEmpty())
	{
		return false;
	}

	for (int i = 0; i < MapNames.Num(); ++i) {
		MapNames[i].ParseIntoArray(temp, TEXT("/"), true);
		MapNames[i] = temp[temp.Num() - 1];
		MapNames[i].ParseIntoArray(temp, TEXT("."), true);
		MapNames[i] = temp[0];
	}

	return true;
}


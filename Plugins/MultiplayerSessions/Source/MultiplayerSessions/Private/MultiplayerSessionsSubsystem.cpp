// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystem.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete))
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		OnlineSessionInterface = Subsystem->GetSessionInterface();
	}

	LastMatchType = "FreeForAll";
}

void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType, bool bIsDedicated)
{
	if (!OnlineSessionInterface.IsValid())
	{
		bSessionCreated = false;
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Red,
			FString::Printf(TEXT("CreateSession(): 1 -- OnlineSessionInterface.IsValid() = %s"), *FString("false"))
		);
		return;
	}
	// Save the NumPublicConnections and MatchType for CreateSessionOnDestroyed
	LastNumPublicConnections = NumPublicConnections;
	LastMatchType = MatchType;
	
	// Remove the old session
	if (OnlineSessionInterface->GetNamedSession(NAME_GameSession))
	{
		bCreateSessionOnDestroy = true;
		DestroySession();

		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Red,
			FString::Printf(TEXT("CreateSession() - faild: Remove the old session"))
		);

		return;
	}

	// Add the delegate into the delegate list and store the handle so we can remove the delegate later
	CreateSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bIsDedicated = bIsDedicated;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bIsLANMatch = true; //false - in steam
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bAllowJoinViaPresence = true;

	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	if (!LocalPlayer ? !OnlineSessionInterface->CreateSession(0, NAME_GameSession, *LastSessionSettings) :
		!OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		bSessionCreated = false;
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}

	GEngine->AddOnScreenDebugMessage(
		-1,
		15.f,
		OnlineSessionInterface.IsValid() ? FColor::Green : FColor::Red,
		FString::Printf(TEXT("CreateSession: 2 -- OnlineSessionInterface.IsValid() = %s"), OnlineSessionInterface.IsValid() ? *FString("true") : *FString("false"))
	);
}

void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	// Search for game sessions
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}

	// Add the delegate into the delegate list
	FindSessionsCompleteDelegateHandle = OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	// Search for sessions
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->bIsLanQuery = true; //false - in steam
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->QuerySettings.Set(FName(TEXT("PRESENCESEARCH")), true, EOnlineComparisonOp::Equals); //SEARCH_PRESENCE = FName(TEXT("PRESENCESEARCH") - don't see #include "OnlineSessionNames.h" so we use FName(TEXT("PRESENCESEARCH")

	
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer ? !OnlineSessionInterface->FindSessions(0, LastSessionSearch.ToSharedRef()) :
		!OnlineSessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		// If we failed to find the session, then we put an empty SessionSearchResult array
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UMultiplayerSessionsSubsystem::StartSession()
{
	const IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld());
	if (!sessionInterface.IsValid())
	{
		MultiplayerOnStartSessionComplete.Broadcast(false);
		return;
	}

	StartSessionCompleteDelegateHandle =
		sessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);

	if (!sessionInterface->StartSession(NAME_GameSession))
	{
		sessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);

		MultiplayerOnStartSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(FBlueprintSessionResult SessionResult)
{
	if (!SessionResult.OnlineResult.IsValid())
	{
		return;
	}
	const FOnlineSessionSearchResult SessionSearchResult = SessionResult.OnlineResult;

	if (!OnlineSessionInterface.IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);

		return;
	}
	// Add the delegate to the delegate list
	JoinSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	// Join the game session
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		return;
	}

	if (OnlineSessionInterface->IsPlayerInSession(NAME_GameSession, *LocalPlayer->GetPreferredUniqueNetId()))
	{
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::AlreadyInSession);
	}
	else if (!OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionSearchResult))
	{
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
	if (!OnlineSessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}
	// Add the delegate to the delegate list
	DestroySessionCompleteDelegateHandle = OnlineSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	// Destroy the session
	if (!OnlineSessionInterface->DestroySession(NAME_GameSession))
	{
		OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
	bCreateSessionOnDestroy = false;
}

void UMultiplayerSessionsSubsystem::FindDedicatedSessions(int32 MaxSearchResults)
{
	FindSessions(MaxSearchResults);
	/*LastSessionSettings->ded*/
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (!OnlineSessionInterface.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Red,
			FString::Printf(TEXT("OnCreateSessionComplete: OnlineSessionInterface.IsValid() = %s"), *FString("false"))
		);
		return;
	}
	// Remove the delegate from the delegate list
	OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	
	if (bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Green,
			FString::Printf(TEXT("OnCreateSessionComplete: bWasSuccessful = true, SessionName = %s"), *SessionName.ToString())
		);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Red,
			FString::Printf(TEXT("OnCreateSessionComplete: bWasSuccessful = %s"), *FString("false"))
		);
	}

	// Broadcast our own custom delegate
	bSessionCreated = bWasSuccessful;
	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}
	// Remove the delegate from the delegate list
	OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

	

	if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}
	
	// Broadcast our own custom delegate
	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!OnlineSessionInterface.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			OnlineSessionInterface.IsValid() ? FColor::Green : FColor::Red,
			FString::Printf(TEXT("OnJoinSessionComplete: %f"), OnlineSessionInterface.IsValid())
		);
		return;
	}
	// Remove the delegate from the delegate list
	OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);



	GEngine->AddOnScreenDebugMessage(
		-1,
		15.f,
		Result == EOnJoinSessionCompleteResult::Success ? FColor::Green : FColor::Red,
		FString::Printf(TEXT("OnJoinSessionComplete: Result = %s"), Result == EOnJoinSessionCompleteResult::Success ? *FString("true") : *FString("false"))
	);
	// Broadcast our own custom delegate
	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	const IOnlineSessionPtr sessionInterface = Online::GetSessionInterface(GetWorld());
	if (sessionInterface)
	{
		sessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
	}

	GEngine->AddOnScreenDebugMessage(
		-1,
		15.f,
		bWasSuccessful ? FColor::Green : FColor::Red,
		FString::Printf(TEXT("OnStartSessionComplete: Result = %s"), bWasSuccessful ? *FString("true") : *FString("false"))
	);

	MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}
	// Remove the delegate from the delegate list
	OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

	// If needs create a new session after destroyed
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType, false);
	}
	// Broadcast our own custom delegate
	bSessionCreated = !bWasSuccessful;
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}


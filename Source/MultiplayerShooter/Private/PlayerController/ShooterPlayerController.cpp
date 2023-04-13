// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerController/ShooterPlayerController.h"
#include "Character/MainCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameMode.h"
#include "GameFramework/PlayerState.h"
#include "PlayerState/ShooterPlayerState.h"
#include "GameMode/ShooterGameMode.h"
#include "GameState/ShooterGameState.h"
#include "ShooterComponents/CombatComponent.h"
#include "HUD/AnnouncementWidget.h"
#include "HUD/ShooterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
	if (ShooterGameState)
	{
		ShooterGameState->OnGameStateChanged.AddUniqueDynamic(this, &AShooterPlayerController::OnMatchStateSet);
	}
	
	CheckMatchState();

	if (SetPlayerHUD())
	{
		RefreshHUD();
		GetWorldTimerManager().SetTimer(GameTimerHandle, this, &AShooterPlayerController::SetHUDTime, 1.0f, true, 0.0f);
	}
}

void AShooterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckTimeSync(DeltaTime);
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (AMainCharacter* MainCharacter = Cast<AMainCharacter>(InPawn))
	{
		MainCharacter->SetIsRespawned();
	}
}

void AShooterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController()) RequestServerTimeFromClient(GetWorld()->GetTimeSeconds());
}

void AShooterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterPlayerController, LevelStartingTime);
	DOREPLIFETIME(AShooterPlayerController, WarmupTime);
	DOREPLIFETIME(AShooterPlayerController, MatchTime);
	DOREPLIFETIME(AShooterPlayerController, CooldownTime);
	DOREPLIFETIME(AShooterPlayerController, MatchState);
}

void AShooterPlayerController::UpdatePlayerHealth(float Health, float MaxHealth)
{
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->HealthBar ||
		!ShooterHUD->GetCharacterOverlay()->HealthText)
	{
		return;
	}

	ShooterHUD->GetCharacterOverlay()->HealthBar->SetPercent(Health / MaxHealth);
	const FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
	ShooterHUD->GetCharacterOverlay()->HealthText->SetText(FText::FromString(HealthText));
}

void AShooterPlayerController::UpdatePlayerScore(float Value)
{
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->Score)
	{
		return;
	}
	
	const FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Value));
	ShooterHUD->GetCharacterOverlay()->Score->SetText(FText::FromString(ScoreText));
}

void AShooterPlayerController::UpdatePlayerDefeats(int32 Value)
{
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->Defeats)
	{
		return;
	}
	
	const FString DefeatsText = FString::Printf(TEXT("%d"), Value);
	ShooterHUD->GetCharacterOverlay()->Defeats->SetText(FText::FromString(DefeatsText));
}

void AShooterPlayerController::DisplayDefeatedMsg()
{
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->DefeatedMsg ||
		!ShooterHUD->GetCharacterOverlay()->DefeatedMsgAnim)
	{
		return;
	}

	UCharacterOverlay* CharacterOverlay = ShooterHUD->GetCharacterOverlay();
	CharacterOverlay->DefeatedMsg->SetVisibility(ESlateVisibility::Visible);
	CharacterOverlay->PlayAnimation(CharacterOverlay->DefeatedMsgAnim);
}

void AShooterPlayerController::UpdateWeaponAmmo(int32 AmmoAmount)
{
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->WeaponAmmoAmount)
	{
		return;
	}
	
	const FString AmmoText = FString::Printf(TEXT("%d"), AmmoAmount);
	ShooterHUD->GetCharacterOverlay()->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
}

void AShooterPlayerController::UpdateCarriedAmmo(int32 AmmoAmount)
{
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->CarriedAmmoAmount)
	{
		return;
	}
	
	const FString AmmoText = FString::Printf(TEXT("%d"), AmmoAmount);
	ShooterHUD->GetCharacterOverlay()->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
}

void AShooterPlayerController::UpdateWeaponType(const FString& WeaponType)
{
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->WeaponType)
	{
		return;
	}
	
	ShooterHUD->GetCharacterOverlay()->WeaponType->SetText(FText::FromString(WeaponType));
}

void AShooterPlayerController::UpdateGrenade(int32 GrenadeAmount)
{
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->GrenadeAmount)
	{
		return;
	}

	const FString GrenadeAmountStr = FString::Printf(TEXT("%d"), GrenadeAmount);
	ShooterHUD->GetCharacterOverlay()->GrenadeAmount->SetText(FText::FromString(GrenadeAmountStr));
}

void AShooterPlayerController::UpdateAnnouncement(int32 Countdown)
{
	if (!SetPlayerHUD() || !ShooterHUD->GetAnnouncement() || !ShooterHUD->GetAnnouncement()->Announce_0 ||
		!ShooterHUD->GetAnnouncement()->Announce_1 || !ShooterHUD->GetAnnouncement()->TimeText)
	{
		return;
	}

	UAnnouncementWidget* Announcement = ShooterHUD->GetAnnouncement();
	if (Countdown <= 0)
	{
		Announcement->Announce_0->SetText(FText());
		Announcement->Announce_1->SetText(FText());
		Announcement->TimeText->SetText(FText());
		return;
	}
	const int32 Minute = Countdown / 60.f;
	const int32 Second = Countdown - 60 * Minute;
	const FString CountdownString = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
	Announcement->TimeText->SetText(FText::FromString(CountdownString));
}

void AShooterPlayerController::UpdateMatchCountDown(int32 Countdown)
{
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->MatchCountdown)
	{
		return;
	}

	UCharacterOverlay* CharacterOverlay = ShooterHUD->GetCharacterOverlay();
	if (Countdown > 0 && Countdown <= 30)
	{
		// Urgent countdown effect, turns red and play blink animation. (animation no need to loop, because update is loop every 1 second)
		CharacterOverlay->MatchCountdown->SetColorAndOpacity((FLinearColor(1.f, 0.f, 0.f)));
		CharacterOverlay->PlayAnimation(CharacterOverlay->TimeBlink);
	}
	else if (Countdown <= 0)
	{
		CharacterOverlay->MatchCountdown->SetText(FText());
		CharacterOverlay->MatchCountdown->SetColorAndOpacity((FLinearColor(1.f, 1.f, 1.f)));
		CharacterOverlay->StopAnimation(CharacterOverlay->TimeBlink);
		return;
	}
	const int32 Minute = Countdown / 60.f;
	const int32 Second = Countdown - 60 * Minute;
	const FString MatchCountdown = FString::Printf(TEXT("%02d:%02d"), Minute, Second);
	/*if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Time left = %s"), *MatchCountdown));*/
	CharacterOverlay->MatchCountdown->SetText(FText::FromString(MatchCountdown));
}

void AShooterPlayerController::UpdateTopScorePlayer()
{
	const AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
	if (!ShooterGameState)
	{
		return;
	}

	TArray<AShooterPlayerState*> PlayerStates = ShooterGameState->GetTopScorePlayerStates();

	if (PlayerStates.IsEmpty())
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("UpdateTopScorePlayer PlayerStates - false")));
		return;
	}
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->TopScorePlayer)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("UpdateTopScorePlayer - false")));
		return;
	}

	
	FString PlayerName;
	for (const auto& State: PlayerStates)
	{
		if (!State) return;
		PlayerName.Append(FString::Printf(TEXT("%s\n"), *State->GetPlayerName()));
	}
	ShooterHUD->GetCharacterOverlay()->TopScorePlayer->SetText(FText::FromString(PlayerName));
}

void AShooterPlayerController::UpdateTopScore()
{
	const AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
	if (!ShooterGameState)
	{
		return;
	}

	const TArray<AShooterPlayerState*> PlayerStates = ShooterGameState->GetTopScorePlayerStates();
	const float TopScore = ShooterGameState->GetTopScore();


	if (PlayerStates.IsEmpty())
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("UpdateTopScore PlayerStates - false")));
		return;
	}
	if (!SetPlayerHUD() || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->TopScore)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("UpdateTopScore - false")));
		return;
	}
	
	FString TopScoreString;
	for (int32 i = 0; i < PlayerStates.Num(); ++i)
	{
		TopScoreString.Append(FString::Printf(TEXT("%d\n"), FMath::CeilToInt32(TopScore)));
	}
	ShooterHUD->GetCharacterOverlay()->TopScore->SetText(FText::FromString(TopScoreString));
}

void AShooterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (!GetWorld())
		return;

	AShooterGameState* GameState = Cast<AShooterGameState>(GetWorld()->GetGameState());
	if (!GameState)
	{
		return;
	}

	if (GameState->GetMatchState() == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
		/*if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("1:	WarmupTime - %f  :	GetServerTime - %f  :	LevelStartingTime - %f"), WarmupTime, GetServerTime(), LevelStartingTime));
	*/}
	else if (GameState->GetMatchState() == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
		/*if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("2:	WarmupTime - %f  :	GetServerTime - %f  :	LevelStartingTime - %f  :	MatchTime - %f"), WarmupTime, GetServerTime(), LevelStartingTime, MatchTime));*/

	}
	else if (GameState->GetMatchState() == MatchState::Cooldown)
	{
		TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
		/*if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("3:	WarmupTime - %f  :	GetServerTime - %f  :	LevelStartingTime - %f  :	MatchTime - %f  :	CooldownTime - %f"), WarmupTime, GetServerTime(), LevelStartingTime, MatchTime, CooldownTime));*/

	}
	const int32 SecondsLeft = FMath::CeilToInt32(TimeLeft);
	if (SecondsLeft != CountdownInt)
	{
		if (GameState->GetMatchState() == MatchState::WaitingToStart || GameState->GetMatchState() == MatchState::Cooldown)
		{
			UpdateAnnouncement(SecondsLeft);
		}
		else if (GameState->GetMatchState() == MatchState::InProgress)
		{
			UpdateMatchCountDown(SecondsLeft);
		}
	}
	if (CountdownInt <= 1 && GameState->GetMatchState() == MatchState::Cooldown)
	{
		AShooterGameMode* CurrentGameMode = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
		if (!CurrentGameMode)
		{
			return;
		}
		CurrentGameMode->ChangeMap();
	}
	CountdownInt = SecondsLeft;
}

void AShooterPlayerController::RefreshHUD()
{
	if (!SetPlayerHUD())
	{
		return;
	}

	RefreshHUDData();

	ShooterHUD->Refresh();
}

void AShooterPlayerController::OpenMenu()
{
	if (!SetPlayerHUD())
	{
		return;
	}

	ShooterHUD->AddMenu();
	SetShowMouseCursor(true);
	UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(this, ShooterHUD->GetMenuWidget(), EMouseLockMode::DoNotLock);
}

void AShooterPlayerController::OnMatchStateSet(FName State)
{
	if (!SetPlayerHUD())
	{
		return;
	}
	HandleMatchState();
}

void AShooterPlayerController::HandleMatchState()
{
	const AShooterGameState* ShooterGameState = Cast<AShooterGameState>(GetWorld()->GetGameState());
	
	if (!ShooterGameState)
	{
		return;
	}

	if (!SetPlayerHUD())
	{
			return;
	}

	if (ShooterGameState->GetMatchState() == MatchState::InProgress)
	{
		if (!ShooterHUD->GetCharacterOverlay())
		{
			ShooterHUD->AddCharacterOverlay();
		}

		if (ShooterHUD->GetAnnouncement())
		{
			ShooterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
		}

		RefreshHUD();
	}
	else if (ShooterGameState->GetMatchState() == MatchState::Cooldown)
	{
		if (!ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetAnnouncement() ||
			!ShooterHUD->GetAnnouncement()->Announce_0 || !ShooterHUD->GetAnnouncement()->Announce_1 ||
			!ShooterHUD->GetAnnouncement()->WinText) return;
		
		ShooterHUD->GetCharacterOverlay()->RemoveFromParent();
		ShooterHUD->GetAnnouncement()->Announce_0->SetText(FText::FromString("New Match Starts in:"));
		ShooterHUD->GetAnnouncement()->Announce_1->SetText(FText::FromString(""));
		ShooterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Visible);

		// When the match ends, we show the winner announcement.
		const AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();

		if (!ShooterPlayerState)
		{
			return;
		}

		FString WinString;
		TArray<AShooterPlayerState*> PlayerStates = ShooterGameState->GetTopScorePlayerStates();
		if (PlayerStates.Num() == 0)
		{
			WinString = "There is no winner.";
		}
		else if (PlayerStates.Num() == 1 && PlayerStates[0] == ShooterPlayerState)
		{
			WinString = "You are the winner!";
		}
		else if (PlayerStates.Num() == 1)
		{
			WinString = "Winner:\n";
		}
		else if (PlayerStates.Num() > 1)
		{
			WinString = "Players tied for the win:\n";
			for (const auto& State: PlayerStates)
			{
				WinString.Append(FString::Printf(TEXT("%s\n"), *State->GetPlayerName()));
			}
		}
		ShooterHUD->GetAnnouncement()->WinText->SetText(FText::FromString(WinString));
		ShooterHUD->GetAnnouncement()->WinText->SetVisibility(ESlateVisibility::Visible);
	}
}

void AShooterPlayerController::RequestServerTimeFromClient_Implementation(float ClientRequestTime)
{
	ReportServerTimeToClient(ClientRequestTime, GetWorld()->GetTimeSeconds());
}

void AShooterPlayerController::ReportServerTimeToClient_Implementation(float ClientRequestTime, float ServerReportTime)
{
	const float CurrClientTime = GetWorld()->GetTimeSeconds();
	const float TripRound = CurrClientTime - ClientRequestTime;
	const float CurrServerTime = ServerReportTime + 0.5f * TripRound;
	SyncDiffTime = CurrServerTime - CurrClientTime;
}

void AShooterPlayerController::CheckTimeSync(float DeltaTime)
{
	SyncRunningTime += DeltaTime;
	if (IsLocalController() && SyncRunningTime > SyncFreq)
	{
		RequestServerTimeFromClient(GetWorld()->GetTimeSeconds());
		SyncRunningTime = 0.f;
	}
}

bool AShooterPlayerController::SetPlayerHUD()
{
	if (!ShooterHUD)
	{
		ShooterHUD = Cast<AShooterHUD>(GetHUD());
		if (!ShooterHUD)
		{
			AShooterGameMode* CurrentGameMode = Cast<AShooterGameMode>(UGameplayStatics::GetGameMode(this));
			if (!CurrentGameMode)
			{
				return false;
			}

			ShooterHUD = Cast<AShooterHUD>(CurrentGameMode->HUDClass.GetDefaultObject());
			if (!ShooterHUD)
			{
				return false;
			}
		}
	}

	return true;
}

void AShooterPlayerController::CheckMatchState()
{
	const AShooterGameMode* ShooterGameMode = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode());
	if (!ShooterGameMode) return;
	
	JoinMidGame(ShooterGameMode->GetLevelStartingTime(), ShooterGameMode->GetWarmupTime(), ShooterGameMode->GetMatchTime(),
		ShooterGameMode->GetCooldownTime(), ShooterGameMode->GetMatchState());
}


void AShooterPlayerController::OnRep_SetLevelStartingTime(float PrevTime)
{
}

void AShooterPlayerController::OnRep_SetWarmupTime(float PrevTime)
{
}

void AShooterPlayerController::OnRep_SetMatchTime(float PrevTime)
{
}

void AShooterPlayerController::OnRep_SetCooldownTime(float PrevTime)
{
}

void AShooterPlayerController::OnRep_SetMatchState(FName PrevState)
{
}

void AShooterPlayerController::Server_JoinMidGame_Implementation(float LevelStarting, float Warmup, float Match, float Cooldown, FName State)
{
	float Prev_LevelStarting = LevelStartingTime;
	float Prev_Warmup = WarmupTime;
	float Prev_Match = MatchTime;
	float Prev_Cooldown = CooldownTime;
	FName Prev_State = MatchState;

	LevelStartingTime = LevelStarting;
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	MatchState = State;

	OnRep_SetLevelStartingTime(Prev_LevelStarting);
	OnRep_SetWarmupTime(Prev_Warmup);
	OnRep_SetMatchTime(Prev_Match);
	OnRep_SetCooldownTime(Prev_Cooldown);
	OnRep_SetMatchState(Prev_State);

}

void AShooterPlayerController::JoinMidGame(float LevelStarting, float Warmup, float Match, float Cooldown, FName State)
{
	Server_JoinMidGame(LevelStarting, Warmup, Match, Cooldown, State);

	// If the player is joining mid-game and the game is now in progress, the UI should switch to the MatchState's UI, so the
	// player should be notified which game state is now going on.
	OnMatchStateSet(MatchState);
}

void AShooterPlayerController::RefreshHUDData()
{
	const AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();

	if (!ShooterPlayerState)
	{
		return;
	}

	UpdatePlayerScore(ShooterPlayerState->GetScore());
	UpdatePlayerDefeats(ShooterPlayerState->GetDefeats());

	const AMainCharacter* MainCharacter = Cast<AMainCharacter>(GetCharacter());

	if (MainCharacter)
	{
		float Health = MainCharacter->GetHealth();
		float MaxHealth = MainCharacter->GetMaxHealth();
		UpdatePlayerHealth(Health, MaxHealth);

		if (MainCharacter->GetCombat())
		{
			UpdateGrenade(MainCharacter->GetCombat()->GetGrenadeAmount());
		}
	}
}

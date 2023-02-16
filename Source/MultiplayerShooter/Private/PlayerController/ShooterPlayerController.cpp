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
#include "HUD/AnnouncementWidget.h"
#include "HUD/ShooterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ShooterHUD = Cast<AShooterHUD>(GetHUD());
	
	CheckMatchState();
}

void AShooterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();

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

void AShooterPlayerController::UpdatePlayerHealth(float Health, float MaxHealth)
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->HealthBar ||
		!ShooterHUD->GetCharacterOverlay()->HealthText) return;
	
	ShooterHUD->GetCharacterOverlay()->HealthBar->SetPercent(Health / MaxHealth);
	const FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
	ShooterHUD->GetCharacterOverlay()->HealthText->SetText(FText::FromString(HealthText));
}

void AShooterPlayerController::UpdatePlayerScore(float Value)
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->Score) return;
	
	const FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Value));
	ShooterHUD->GetCharacterOverlay()->Score->SetText(FText::FromString(ScoreText));
}

void AShooterPlayerController::UpdatePlayerDefeats(int32 Value)
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->Defeats) return;
	
	const FString DefeatsText = FString::Printf(TEXT("%d"), Value);
	ShooterHUD->GetCharacterOverlay()->Defeats->SetText(FText::FromString(DefeatsText));
}

void AShooterPlayerController::DisplayDefeatedMsg()
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->DefeatedMsg ||
		!ShooterHUD->GetCharacterOverlay()->DefeatedMsgAnim) return;

	UCharacterOverlay* CharacterOverlay = ShooterHUD->GetCharacterOverlay();
	CharacterOverlay->DefeatedMsg->SetVisibility(ESlateVisibility::Visible);
	CharacterOverlay->PlayAnimation(CharacterOverlay->DefeatedMsgAnim);
}

void AShooterPlayerController::UpdateWeaponAmmo(int32 AmmoAmount)
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->WeaponAmmoAmount) return;
	
	const FString AmmoText = FString::Printf(TEXT("%d"), AmmoAmount);
	ShooterHUD->GetCharacterOverlay()->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
}

void AShooterPlayerController::UpdateCarriedAmmo(int32 AmmoAmount)
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->CarriedAmmoAmount) return;
	
	const FString AmmoText = FString::Printf(TEXT("%d"), AmmoAmount);
	ShooterHUD->GetCharacterOverlay()->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
}

void AShooterPlayerController::UpdateWeaponType(const FString& WeaponType)
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->WeaponType) return;
	
	ShooterHUD->GetCharacterOverlay()->WeaponType->SetText(FText::FromString(WeaponType));
}

void AShooterPlayerController::UpdateGrenade(int32 GrenadeAmount)
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->GrenadeAmount) return;

	const FString GrenadeAmountStr = FString::Printf(TEXT("%d"), GrenadeAmount);
	ShooterHUD->GetCharacterOverlay()->GrenadeAmount->SetText(FText::FromString(GrenadeAmountStr));
}

void AShooterPlayerController::UpdateAnnouncement(int32 Countdown)
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetAnnouncement() || !ShooterHUD->GetAnnouncement()->Announce_0 ||
		!ShooterHUD->GetAnnouncement()->Announce_1 || !ShooterHUD->GetAnnouncement()->TimeText) return;

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
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->MatchCountdown) return;

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
	CharacterOverlay->MatchCountdown->SetText(FText::FromString(MatchCountdown));
}

void AShooterPlayerController::UpdateTopScorePlayer()
{
	const AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
	if (!ShooterGameState) return;

	auto PlayerStates = ShooterGameState->GetTopScorePlayerStates();
	if (PlayerStates.IsEmpty()) return;

	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->TopScorePlayer) return;
	
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
	if (!ShooterGameState) return;

	const auto PlayerStates = ShooterGameState->GetTopScorePlayerStates();
	const float TopScore = ShooterGameState->GetTopScore();
	if (PlayerStates.IsEmpty()) return;
	
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD || !ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetCharacterOverlay()->TopScore) return;
	
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
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
	}
	const int32 SecondsLeft = FMath::CeilToInt32(TimeLeft);
	if (SecondsLeft != CountdownInt)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			UpdateAnnouncement(SecondsLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			UpdateMatchCountDown(SecondsLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

void AShooterPlayerController::RefreshHUD()
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (ShooterHUD) ShooterHUD->Refresh();
}

void AShooterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	HandleMatchState();
}

void AShooterPlayerController::HandleMatchState()
{
	ShooterHUD = ShooterHUD ? ShooterHUD : Cast<AShooterHUD>(GetHUD());
	if (!ShooterHUD) return;
	
	if (MatchState == MatchState::InProgress)
	{
		if (!ShooterHUD->GetCharacterOverlay()) ShooterHUD->AddCharacterOverlay();

		if (ShooterHUD->GetAnnouncement())
		{
			ShooterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		if (!ShooterHUD->GetCharacterOverlay() || !ShooterHUD->GetAnnouncement() ||
			!ShooterHUD->GetAnnouncement()->Announce_0 || !ShooterHUD->GetAnnouncement()->Announce_1 ||
			!ShooterHUD->GetAnnouncement()->WinText) return;
		
		ShooterHUD->GetCharacterOverlay()->RemoveFromParent();
		ShooterHUD->GetAnnouncement()->Announce_0->SetText(FText::FromString("New Match Starts in:"));
		ShooterHUD->GetAnnouncement()->Announce_1->SetText(FText::FromString(""));
		ShooterHUD->GetAnnouncement()->SetVisibility(ESlateVisibility::Visible);

		// When the match ends, we show the winner announcement.
		const AShooterGameState* ShooterGameState = Cast<AShooterGameState>(UGameplayStatics::GetGameState(this));
		const AShooterPlayerState* ShooterPlayerState = GetPlayerState<AShooterPlayerState>();
		if (!ShooterGameState || !ShooterPlayerState) return;

		FString WinString;
		auto PlayerStates = ShooterGameState->GetTopScorePlayerStates();
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
			WinString.Append(FString::Printf(TEXT("%s\n"), *PlayerStates[0]->GetPlayerName()));
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

void AShooterPlayerController::CheckMatchState()
{
	const AShooterGameMode* ShooterGameMode = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode());
	if (!ShooterGameMode) return;
	
	JoinMidGame(ShooterGameMode->GetLevelStartingTime(), ShooterGameMode->GetWarmupTime(), ShooterGameMode->GetMatchTime(),
		ShooterGameMode->GetCooldownTime(), ShooterGameMode->GetMatchState());
}

void AShooterPlayerController::JoinMidGame(float LevelStarting, float Warmup, float Match, float Cooldown, FName State)
{
	LevelStartingTime = LevelStarting;
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	MatchState = State;
	
	// If the player is joining mid-game and the game is now in progress, the UI should switch to the MatchState's UI, so the
	// player should be notified which game state is now going on.
	OnMatchStateSet(MatchState);
}

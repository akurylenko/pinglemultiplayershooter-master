// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/GameMenuWidget.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "SaveGame/SaveGameSubsystem.h"
#include "PlayerState/ShooterPlayerState.h"


void UGameMenuWidget::Resume()
{
	GetOwningPlayer()->SetShowMouseCursor(false);
	UWidgetBlueprintLibrary::SetInputMode_GameOnly(GetOwningPlayer());

	RemoveFromParent();
}

void UGameMenuWidget::Exit()
{
	RemoveSaves();

	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, true);
}

void UGameMenuWidget::RemoveSaves()
{
	GetOwningPlayer()->SetShowMouseCursor(false);
	USaveGameSubsystem* SG = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	AShooterPlayerState* PS = Cast<AShooterPlayerState>(GetOwningPlayer()->PlayerState);

	SG->RemoveSaveGame(PS);
}

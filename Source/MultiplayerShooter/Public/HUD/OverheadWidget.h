// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSHOOTER_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void UpdateHealtBar(float Health, float MaxHealth);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerName(APawn* InPawn);

protected:
	void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld);

	UFUNCTION(BlueprintCallable)
	void ShowPlayerNetRole(APawn* InPawn);

private:
	virtual bool Initialize() override;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DisplayText;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class USizeBox* HealthBarBox;

	void SetDisplayText(FString TextToDisplay);
	
};

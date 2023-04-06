// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSHOOTER_API UGameMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = GameMenu)
	void Resume();

	UFUNCTION(BlueprintCallable, Category = GameMenu)
	void Exit();

protected:
	UFUNCTION(BlueprintCallable, Category = GameMenu)
	void RemoveSaves();
};

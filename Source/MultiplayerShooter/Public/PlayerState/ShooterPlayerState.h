// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShooterPlayerState.generated.h"

class UMultiShooterSaveGame;

UCLASS()
class MULTIPLAYERSHOOTER_API AShooterPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void PostInitializeComponents() override;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	void UpdateScore();

	void UpdateDefeats();

	void SetDefeats(int32 NewDefeats);

	UFUNCTION(BlueprintGetter)
	int32 GetDefeats() const { return Defeats; }

	UFUNCTION(BlueprintNativeEvent)
	void SavePlayerState(UMultiShooterSaveGame* SaveObject);

	UFUNCTION(BlueprintNativeEvent)
	void LoadPlayerState(UMultiShooterSaveGame* SaveObject);

	UFUNCTION(BlueprintNativeEvent)
	void FreePlayerState(UMultiShooterSaveGame* SaveObject);

private:
	UPROPERTY(EditAnywhere)
	float ScoreAmount = 5.f;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_SetDefeats)
	int32 Defeats = 0;

	UPROPERTY()
	class AShooterPlayerController* ShooterPlayerController;

protected:
	UFUNCTION()
	void OnGameClosed();

	UFUNCTION()
	void OnRep_SetDefeats(int32 PrevDefeats);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetDefeats(int32 NewDefeats);
};

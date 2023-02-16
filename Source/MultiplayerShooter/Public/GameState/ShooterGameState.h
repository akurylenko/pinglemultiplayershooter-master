// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ShooterGameState.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSHOOTER_API AShooterGameState : public AGameState
{
	GENERATED_BODY()

public:
	/** Once a player is eliminated, we then need to update the array: TopScorePlayerStates and update the TopScore player in the HUD */
	void UpdateTopScorePlayerStates(class AShooterPlayerState* PlayerState);


private:
	UPROPERTY()
	float TopScore = 0.f;

	/** The common code within OnRep_TopScore() */
	void HandleTopScore();

	/** An array contains the top score players' states */
	UPROPERTY()
	TArray<class AShooterPlayerState*> TopScorePlayerStates;

	/** The common code within OnRep_TopScorePlayerStates() */
	void HandleTopScorePlayerStates();

public:
	FORCEINLINE float GetTopScore() const { return TopScore; }
	 FORCEINLINE const TArray<class AShooterPlayerState*>& GetTopScorePlayerStates() const { return TopScorePlayerStates; }
};

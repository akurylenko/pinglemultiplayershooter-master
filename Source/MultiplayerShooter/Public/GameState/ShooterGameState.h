// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ShooterGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameStateChanged, FName, State);


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

protected:

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_HandleTopScore)
	float TopScore = 0.f;

	/** The common code within OnRep_TopScore() */
	void HandleTopScore(float Score);

	UFUNCTION(Server, Reliable)
	void Server_HandleTopScore(float Score);

	UFUNCTION()
	void OnRep_HandleTopScore(float Score);

	/** An array contains the top score players' states */
	UPROPERTY(ReplicatedUsing = OnRep_HandleTopScorePlayerStates)
	TArray<class AShooterPlayerState*> TopScorePlayerStates;

	/** The common code within OnRep_TopScorePlayerStates() */
	void HandleTopScorePlayerStates(class AShooterPlayerState* PlayerState, bool bRewrite);

	UFUNCTION(Server, Reliable)
	void Server_HandleTopScorePlayerStates(class AShooterPlayerState* PlayerState, bool bRewrite);

	UFUNCTION()
	void OnRep_HandleTopScorePlayerStates(TArray<class AShooterPlayerState*> TopScorePS);

public:
	FORCEINLINE float GetTopScore() const { return TopScore; }
	 FORCEINLINE const TArray<class AShooterPlayerState*>& GetTopScorePlayerStates() const { return TopScorePlayerStates; }

	 UPROPERTY(BlueprintAssignable)
	 FOnGameStateChanged OnGameStateChanged;

	 virtual void OnRep_MatchState() override;
};

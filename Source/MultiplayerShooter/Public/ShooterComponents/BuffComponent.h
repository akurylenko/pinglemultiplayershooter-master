// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MULTIPLAYERSHOOTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	friend class AMainCharacter;
	UBuffComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/* Healing */
	void Heal(float Amount, float Duration);

	/* Speeding up */
	void SpeedUp(float Scale, float Duration);

protected:
	virtual void BeginPlay() override;

private:
	/* Initialized in MainCharacter.cpp, PostInitializeComponents(). */
	UPROPERTY()
	class AMainCharacter* MainCharacter;


	/**
	 *	Healing
	 */


	UPROPERTY(EditAnywhere, Category = Health)
	float HealingTimerRate = 0.02f;
	
	FTimerHandle HealingTimerHandle;
	float HealthAmount;
	float HealthDuration;
	float TimeRemaining;
	void Healing();


	/**
	 *	Speeding
	 */

	
	FTimerHandle SpeedingTimerHandle;
	float BaseWalkSpeed;
	float BaseWalkSpeedCrouched;
	float AimWalkSpeed;
	float AimWalkSpeedCrouched;
	void ResetSpeed();
};

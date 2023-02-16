// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterComponents/BuffComponent.h"
#include "Character/MainCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterComponents/CombatComponent.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuffComponent::Heal(float Amount, float Duration)
{
	TimeRemaining = Duration;
	HealthAmount = Amount;
	HealthDuration = Duration;
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HealingTimerHandle, this, &UBuffComponent::Healing, HealingTimerRate, true);
	}
}

void UBuffComponent::SpeedUp(float Scale, float Duration)
{
	if (MainCharacter)
	{
		if (UCombatComponent* Combat = MainCharacter->GetCombat())
		{
			BaseWalkSpeed = Combat->GetBaseWalkSpeed();
			BaseWalkSpeedCrouched = Combat->GetBaseWalkSpeedCrouched();
			AimWalkSpeed = Combat->GetAimWalkSpeed();
			AimWalkSpeedCrouched = Combat->GetAimWalkSpeedCrouched();
			Combat->SetBaseWalkSpeed(Scale * BaseWalkSpeed);
			Combat->SetBaseWalkSpeedCrouched(Scale * BaseWalkSpeedCrouched);
			Combat->SetAimWalkSpeed(Scale * AimWalkSpeed);
			Combat->SetAimWalkSpeedCrouched(Scale * AimWalkSpeedCrouched);

			// Change the character movement speed
			Combat->UpdateCharacterSpeed();
		}
	}
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SpeedingTimerHandle, this, &UBuffComponent::ResetSpeed, Duration, false);
	}
}

void UBuffComponent::Healing()
{
	if (!MainCharacter) return;
	
	// Time's up, stop healing. Or if character's health is full, then we stop healing.
	TimeRemaining -= HealingTimerRate;
	if (TimeRemaining < 0.f || MainCharacter->GetHealth() >= MainCharacter->GetMaxHealth())
	{
		if (const UWorld* World = GetWorld())
		{
			// Clear the timer and immediately return, or it will continue to executed the rest part of the code.
			World->GetTimerManager().ClearTimer(HealingTimerHandle);
			return;
		}
	}
	// Calculate the amount of healing per timer called.
	const float HealingAmountPerTick = HealthAmount * HealingTimerRate / HealthDuration;
	MainCharacter->SetHealth(MainCharacter->GetHealth() + HealingAmountPerTick);
}

void UBuffComponent::ResetSpeed()
{
	if (MainCharacter)
	{
		if (UCombatComponent* Combat = MainCharacter->GetCombat())
		{
			Combat->SetBaseWalkSpeed(BaseWalkSpeed);
			Combat->SetBaseWalkSpeedCrouched(BaseWalkSpeedCrouched);
			Combat->SetAimWalkSpeed(AimWalkSpeed);
			Combat->SetAimWalkSpeedCrouched(AimWalkSpeedCrouched);
			Combat->UpdateCharacterSpeed();
		}
	}
}

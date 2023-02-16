// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ShooterMovementComponent.h"

UProjectileMovementComponent::EHandleBlockingHitResult UShooterMovementComponent::HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);

	// We force the projectile to keep moving when it hits something.
	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void UShooterMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// Rockets should not stop; only explode when their collision box detects a hit.
}
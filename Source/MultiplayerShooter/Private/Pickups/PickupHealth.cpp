// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/PickupHealth.h"
#include "Character/MainCharacter.h"
#include "ShooterComponents/BuffComponent.h"
#include "Components/SphereComponent.h"

APickupHealth::APickupHealth()
{
	if (PickupCollision)
	{
		PickupCollision->SetSphereRadius(80.f);
	}
}

void APickupHealth::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Heal
	if (const AMainCharacter* MainCharacter = Cast<AMainCharacter>(OtherActor))
	{
		if (UBuffComponent* Buff = MainCharacter->GetBuff())
		{
			Buff->Heal(HealingAmount, Duration);
		}
	}

	// Destroy
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

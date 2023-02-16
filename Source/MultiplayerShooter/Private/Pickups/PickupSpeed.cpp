// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/PickupSpeed.h"

#include "ShooterComponents/BuffComponent.h"
#include "Character/MainCharacter.h"
#include "Components/SphereComponent.h"

APickupSpeed::APickupSpeed()
{
	if (PickupCollision)
	{
		PickupCollision->SetSphereRadius(80.f);
	}
}

void APickupSpeed::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Speed Buff
	if (const AMainCharacter* MainCharacter = Cast<AMainCharacter>(OtherActor))
	{
		if (UBuffComponent* Buff = MainCharacter->GetBuff())
		{
			Buff->SpeedUp(SpeedBuff, Duration);
		}
	}
	//Destroy
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

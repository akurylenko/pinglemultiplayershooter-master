// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

UCLASS()
class MULTIPLAYERSHOOTER_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	APickup();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	/* Set the scale and the extent in child class. */
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* PickupCollision;

private:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* SceneComponent;
	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(VisibleAnywhere, Category = Effect)
	class UNiagaraComponent* NiagaraComponent;

	UPROPERTY(EditAnywhere, Category = Effect)
	class UNiagaraSystem* NiagaraEffect;

	UPROPERTY(EditAnywhere, Category = Effect)
	class USoundCue* SoundPickup;

	UPROPERTY(EditAnywhere, Category = Effect)
	float BaseTurnRate = 45.f;

	FTimerHandle TurnTimerHandle;

	UPROPERTY(EditAnywhere, Category = Effect)
	float TurnTimerRate = 0.02f;
	
	void Turn();

	UFUNCTION()
	void SpawnBuffEffectAttached(class AMainCharacter* AttachedCharacter) const;
};

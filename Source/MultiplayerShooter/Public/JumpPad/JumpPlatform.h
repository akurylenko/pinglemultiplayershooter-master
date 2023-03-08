// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "JumpPlatform.generated.h"

UCLASS()
class MULTIPLAYERSHOOTER_API AJumpPlatform : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AJumpPlatform();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Components)
	UStaticMeshComponent* RootMesh;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = JumpVelocity)
	float ZLaunchVelocity;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpPad/JumpPlatform.h"
#include "Character/MainCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"

// Sets default values
AJumpPlatform::AJumpPlatform()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CylinderComponent"));
	RootComponent = RootMesh;

	RootMesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	ZLaunchVelocity = 1250.0f;
}

// Called when the game starts or when spawned
void AJumpPlatform::BeginPlay()
{
	Super::BeginPlay();
	
	RootMesh->OnComponentBeginOverlap.AddDynamic(this, &AJumpPlatform::OnBeginOverlap);
}

void AJumpPlatform::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMainCharacter* OurCharacter = Cast<AMainCharacter>(OtherActor);

	if (OurCharacter)
	{
		OurCharacter->LaunchCharacter(FVector(0.0f, 0.0f, ZLaunchVelocity), false, true);
	}
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"
#include "Weapon/ShooterMovementComponent.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileRocket::AProjectileRocket()
{
	// Create a new customized projectile movement component.
	ShooterMovementComponent = CreateDefaultSubobject<UShooterMovementComponent>(TEXT("Rocket Movement Component"));
	ShooterMovementComponent->bRotationFollowsVelocity = true;
	ShooterMovementComponent->InitialSpeed = 2000.f;
	ShooterMovementComponent->MaxSpeed = 2000.f;
	ShooterMovementComponent->ProjectileGravityScale = 0.f;

	// Destroyed and explode when it hits something.
	bOnHitDestroy = true;	
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	// Spawn a niagara system component for control the niagara system.
	SpawnTrailSystem();
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// The Owner/Instigator is set in SpawnParams when we spawn the projectile.
	const APawn* ProjectileInstigator = GetInstigator();
	if (!ProjectileInstigator) return;

	// If we hit ourselves, it'll not trigger the HitImpact.
	if (OtherActor == GetOwner()) return;

	// ApplyDamage logic
	UGameplayStatics::ApplyRadialDamageWithFalloff(this, Damage, MinDamage, GetActorLocation(), DamageInnerRadius, DamageOuterRadius, DamageFalloff,
	UDamageType::StaticClass(), TArray<AActor*>(), this, ProjectileInstigator->GetController());

	// We use the niagara system instead of particle system in the parent, so we need to override part of the functionality.
	// We should deactivate to stop keeping generating the particle effect.
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
	{
		TrailSystemComponent->GetSystemInstanceController()->Deactivate();
	}
	
	// Super::OnHit be called, but destroy need to be manually called after the timer finished.
	Super::OnHit(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
}

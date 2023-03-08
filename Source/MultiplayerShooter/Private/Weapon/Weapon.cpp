// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "ShooterComponents/CombatComponent.h"
#include "Character/MainCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "PlayerController/ShooterPlayerController.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon Mesh"));
	SetRootComponent(WeaponMesh);

	// Collision Preset --- Block all the other things except the pawn (so we can step over it once drop the weapon),
	// and the camera to avoid zooming effect.
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Area Sphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// Enable it when we on the server machine

	// Render -- Custom Depth Stencil
	WeaponMesh->SetRenderCustomDepth(true);
	WeaponMesh->SetCustomDepthStencilValue(DEPTH_PURPLE);
	
	// Widget configuration.
	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Weapon Widget"));
	PickupWidget->SetupAttachment(RootComponent);
	PickupWidget->SetVisibility(false);

	CrosshairsMaxSpread = 64.f;
	CrosshairsMinSpread = 5.f;
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereBeginOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& TraceHitTarget)
{
	// This is common logic, feature can be added by override.
	if (FireAnimation && WeaponMesh)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}
	SpendRound();
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	
	HandleWeaponState();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	const FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
	DetachFromActor(DetachmentTransformRules);
	SetOwner(nullptr);
	ResetOwnership();
}

void AWeapon::SpendRound()
{	
	SetAmmo(Ammo - 1);	
}


void AWeapon::OnRep_SetAmmo(const int32 PrevAmount)
{
	HandleAmmo();
}

bool AWeapon::Server_SetAmmo_Validate(const int32 Amount)
{
	return true;
}

void AWeapon::Server_SetAmmo_Implementation(const int32 Amount)
{
	int32 PrevAmmo = Ammo;
	Ammo = Amount;

	OnRep_SetAmmo(PrevAmmo);
}

void AWeapon::SetAmmo(const int32 Amount)
{
	if (Amount < 0 || Amount > ClipSize) return;
	
	Server_SetAmmo(Amount);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::HandleAmmo()
{
	SetHUDAmmo();

	WeaponOwnerCharacter = WeaponOwnerCharacter ? WeaponOwnerCharacter : Cast<AMainCharacter>(GetOwner());
	
	// Jump to the end section of animation when the clip is full when reloading the shotgun.
	if (WeaponOwnerCharacter->GetCombat() && WeaponType == EWeaponType::EWT_Shotgun && IsAmmoFull())
	{
		WeaponOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
}

void AWeapon::SetHUDAmmo()
{
	WeaponOwnerCharacter = WeaponOwnerCharacter ? WeaponOwnerCharacter : Cast<AMainCharacter>(GetOwner());
	if (!WeaponOwnerCharacter) return;

	WeaponOwnerController = WeaponOwnerController ? WeaponOwnerController : Cast<AShooterPlayerController>(WeaponOwnerCharacter->Controller);
	if (!WeaponOwnerController) return;
	
	WeaponOwnerController->UpdateWeaponAmmo(Ammo);
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	
	// Recover the ownership, otherwise if a player drops the weapon and another player picks up the weapon, it will
	// update the other player's HUD. 
	if (GetOwner() == nullptr) ResetOwnership();
	else SetHUDAmmo();
}

void AWeapon::ResetOwnership()
{
	WeaponOwnerCharacter = nullptr;
	WeaponOwnerController = nullptr;
}

void AWeapon::SetCustomDepth(const bool bEnabled)
{
	if (!WeaponMesh) return;

	WeaponMesh->SetRenderCustomDepth(bEnabled);
	if (bEnabled)
	{
		WeaponMesh->SetCustomDepthStencilValue(DEPTH_PURPLE);
		WeaponMesh->MarkRenderStateDirty();
	}
}

void AWeapon::HandleWeaponState()
{
	// Change the weapon's pickup widget in Server World to be invisible 
	switch(WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// If weapon type is SMG, we just disable the physics simulation of the weapon mesh because physics simulation is incompatible with attaching actor behavior.
		// But it doesn't affect the strap physics simulation in Weapon Mesh-->Physics Asset-->Physics Type in blueprint.
		if (WeaponType == EWeaponType::EWT_SMG)
		{
			WeaponMesh->SetSimulatePhysics(false);
		}
		else
		{
			// Set physics simulation, be aware of the sequence.
			WeaponMesh->SetSimulatePhysics(false);
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		// Cancel the stencil -- Outline effect when equipping the weapon.
		SetCustomDepth(false);
		break;
		
	case EWeaponState::EWS_Dropped:
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		
		// Set physics simulation, be aware of the sequence.
		WeaponMesh->SetSimulatePhysics(true);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// Enable the stencil -- Outline effect when the weapon is dropped to attract the players.
		SetCustomDepth(true);
		break;

	case EWeaponState::EWS_Initial:
		break;
		
	case EWeaponState::EWS_Max:
		break;
	}
}

void AWeapon::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AMainCharacter* MainCharacter = Cast<AMainCharacter>(OtherActor))
	{
		MainCharacter->SetOverlappingWeapon(this); 
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AMainCharacter* MainCharacter = Cast<AMainCharacter>(OtherActor))
	{
		MainCharacter->SetOverlappingWeapon(nullptr); 
	}
}


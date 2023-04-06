// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MainCharacter.h"
#include "ShooterComponents/CombatComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/Weapon.h"
#include "MultiplayerShooter/MultiplayerShooter.h"
#include "ShooterComponents/BuffComponent.h"
#include "GameMode/ShooterGameMode.h"
#include "HUD/CharacterOverlay.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "PlayerController/ShooterPlayerController.h"
#include "HUD/OverheadWidget.h"

AMainCharacter::AMainCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Follow Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Overhead Widget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat Component"));
	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("Buff Component"));

	TimelineComponent = CreateDefaultSubobject<UTimelineComponent>(TEXT("Timeline Component"));

	// Grenade attachment to the socket and set its collision
	GrenadeAttached = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeAttached"));
	GrenadeAttached->SetupAttachment(GetMesh(), FName("GrenadeAttached"));
	GrenadeAttached->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Avoid the zooming effect (camera overlaps with the character)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionObjectType(ECC_SKELETAL_MESH);

	// Initialize the character status.
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;

	// Net update frequency.
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	LastAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
	GrenadeAttached->SetVisibility(false);
	OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);

	MyOverheadWidget = CreateWidget<UOverheadWidget>(GetWorld()->GetFirstPlayerController(), MyOverheadWidgetClass);

	UpdateOverHeadWidget();

}

void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	AimOffset(DeltaTime);
	
	FABRIK_IK_LeftHand();

	CorrectWeaponRotation(DeltaTime);

	HideCharacterIfClose();
}

void AMainCharacter::AimOffset(float DeltaTime)
{
	if (Combat && !Combat->EquippedWeapon) return;
	
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	const float Speed = Velocity.Size();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();

	if(Speed == 0.f && !bIsInAir)
	{
		// Default set to false, we set it to true in 'TurnInPlace' function.
		bUseControllerRotationYaw = false;

		// If we aim, we turn in place when needed, if we just click aim once, we should wait for the 'Turn in place' finished
		// so we need to check the TurningInPlace condition.
		if (IsAiming() || TurningInPlace != ETurningInPlace::ETIP_NotTurning) TurnInPlace(DeltaTime);
	}
	else if (Speed > 0.f || bIsInAir)
	{
		// Reset the AimOffset-related properties
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		LastAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
	}
	AO_Pitch = GetBaseAimRotation().Pitch;

	// Due to Package issue over the network, we need to fix it on the AO_Pitch.
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		const FVector2D InRange(270.f, 360.f);
		const FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void AMainCharacter::TurnInPlace(float DeltaTime)
{
	// 1. Offset the 'RotateRootBone' node in AnimBP when in idle
	// 2. When the turning rotation is too big like 150 degrees, we can make a trick by 'bUseControllerRotationYaw' to make the character looks like
	// he rotates 150 degrees even though we actually make him rotate only 90 degrees.
	bUseControllerRotationYaw = true;

	if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
	{
		const FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, LastAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		Interp_AO_Yaw = AO_Yaw;
		if (AO_Yaw > 90.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (AO_Yaw < -90.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
	}
	else
	{
		Interp_AO_Yaw = FMath::FInterpTo(Interp_AO_Yaw, 0, DeltaTime, 10.f);
		AO_Yaw = Interp_AO_Yaw;
		if (FMath::Abs(Interp_AO_Yaw) <  1.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			LastAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void AMainCharacter::FABRIK_IK_LeftHand()
{
	if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh() && GetMesh())
	{
		// Get the socket transform in the world space
		LeftHandTransform = Combat->EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		// Transform the socket into the bone space ---- relative to the bone 'hand_r'
		GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
	}
}

void AMainCharacter::CorrectWeaponRotation(float DeltaTime)
{
	// TraceUnderCorsshairs is a machine-related function, so the HitResult cannot be known by the other machine unless by RPC.
	if (!Combat) return;
	
	
	// To adjust the hand rotation, to nearly match the two trace line, not precisely match. (This is a trick)
	if (!Combat->EquippedWeapon || !Combat->EquippedWeapon->GetWeaponMesh()) return;
	const FTransform RightHandTransform = Combat->EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Combat->HitTarget, RightHandTransform.GetLocation());
	RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 20.f);
}

void AMainCharacter::HideCharacterIfClose()
{
	if (!IsLocallyControlled()) return;
	if (!FollowCamera || !GetMesh()) return;
	if (!Combat || !Combat->EquippedWeapon || !Combat->EquippedWeapon->GetWeaponMesh()) return;
	
	const bool bHideCharacter = ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold);
	GetMesh()->SetVisibility(!bHideCharacter);

	// When the camera is close to the character, we make the character and the weapon invisible, so in this situation when the character is eliminated,
	// the weapon is dropped and it's still invisible, so we need to recover its visibility.
	const bool bHideWeapon = bHideCharacter && Combat->EquippedWeapon->GetWeaponState() == EWeaponState::EWS_Equipped;
	Combat->EquippedWeapon->GetWeaponMesh()->SetVisibility(!bHideWeapon);
}

void AMainCharacter::Eliminated()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}

	// Disable the collision.
	if (GetCapsuleComponent()) GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (GetMesh()) GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Disable the movement of inertial even though we lose the control. 
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
	}
	ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(Controller);
	if (ShooterPlayerController)
	{
		// Play the defeated HUD effect
		ShooterPlayerController->DisplayDefeatedMsg();

		// Disable the controller input.
		DisableInput(ShooterPlayerController);

		// We need to make sure bFireButtonPressed is cleared or the weapon will keep firing because the input is disabled
		FireButtonReleased();
	}
	IsAiming() ? PlayDeathIronMontage() : PlayDeathHipMontage();

	StartDissolve();
	PlayElimBotEffect();

	OverheadWidget->DestroyComponent(true);
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &ThisClass::RespawnTimerFinished, TimerDelay, false);
}

void AMainCharacter::Multicast_UpdateOverHeadWidget_Implementation()
{
	if (IsValid(MyOverheadWidget))
	{
		if (IsLocallyControlled())
		{
			MyOverheadWidget->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			MyOverheadWidget->SetVisibility(ESlateVisibility::Visible);

			MyOverheadWidget->UpdateHealtBar(Health, MaxHealth);
			MyOverheadWidget->ShowPlayerName(this);
		}
		OverheadWidget->SetWidget(MyOverheadWidget);
	}
}

void AMainCharacter::UpdateOverHeadWidget()
{
	if (GetLocalRole() == ROLE_SimulatedProxy || 
		GetLocalRole() == ROLE_AutonomousProxy)
	{
		if (IsValid(MyOverheadWidget))
		{
			if (IsLocallyControlled())
			{
				MyOverheadWidget->SetVisibility(ESlateVisibility::Hidden);
			}
			else
			{
				MyOverheadWidget->SetVisibility(ESlateVisibility::Visible);

				MyOverheadWidget->UpdateHealtBar(Health, MaxHealth);
				MyOverheadWidget->ShowPlayerName(this);
			}
			OverheadWidget->SetWidget(MyOverheadWidget);
		}
	}
	else if (GetLocalRole() == ROLE_Authority ||
			 GetLocalRole() == ROLE_MAX)
	{
		Multicast_UpdateOverHeadWidget();
	}
}

void AMainCharacter::RespawnTimerFinished()
{
	if (!GetWorld()) return;
	
	if (AShooterGameMode* ShooterGameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>())
	{
		ShooterGameMode->RequestRespawn(this, Controller);
	}
}

void AMainCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	// If the character is eliminated, then directly return to avoid spam, or the character will be spawned multiple times.
	if (Health <= 0.f) return;
	
	SetHealth(FMath::Clamp(Health - Damage, 0.f, MaxHealth));
	if (Health <= 0.f && GetWorld())
	{
		if (AShooterGameMode* ShooterGameMode = GetWorld()->GetAuthGameMode<AShooterGameMode>())
		{
			ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(Controller);
			AShooterPlayerController* AttackerController = Cast<AShooterPlayerController>(InstigatedBy);
			ShooterGameMode->PlayerEliminated(this, ShooterPlayerController, AttackerController);
		}
	}
}

void AMainCharacter::SetHUDHealth()
{
	ShooterPlayerController = ShooterPlayerController ? ShooterPlayerController : Cast<AShooterPlayerController>(Controller);
	if (!ShooterPlayerController) return;

	ShooterPlayerController->UpdatePlayerHealth(Health, MaxHealth);

	UpdateOverHeadWidget();
}

void AMainCharacter::PlayHitReactMontage() const
{
	// If health <= 0.f, it should play the eliminated animation rather than the hit react.
	if (!Combat || !Combat->EquippedWeapon || Health <= 0.f) return;

	PlayMontage(HitReactMontage, FName("FromLeft"));
}

void AMainCharacter::PlayDeathHipMontage() const
{
	// The montage is played only when the character is holding a weapon.
	if (!Combat || !Combat->EquippedWeapon) return;
	
	PlayMontage(DeathHipMontage, FName("Death Hip 1"));
}

void AMainCharacter::PlayDeathIronMontage() const
{
	// The montage is played only when the character is holding a weapon.
	if (!Combat || !Combat->EquippedWeapon) return;

	PlayMontage(DeathIronMontage, FName("Death Iron 1"));
}

void AMainCharacter::PlayReloadMontage() const
{
	if (!Combat || !Combat->EquippedWeapon) return;

	FName SectionName;
	switch (Combat->EquippedWeapon->GetWeaponType())
	{
	case EWeaponType::EWT_AssaultRifle:
		SectionName = FName("AssaultRifle");
		break;
	case EWeaponType::EWT_RocketLauncher:
		SectionName = FName("RocketLauncher");
		break;
	case EWeaponType::EWT_Pistol:
		SectionName = FName("Pistol");
		break;
	case EWeaponType::EWT_SMG:
		SectionName = FName("SMG");
		break;
	case EWeaponType::EWT_Shotgun:
		SectionName = FName("Shotgun");
		break;
	case EWeaponType::EWT_SniperRifle:
		SectionName = FName("SniperRifle");
		break;
	case EWeaponType::EWT_GrenadeLauncher:
		SectionName = FName("GrenadeLauncher");
		break;
	case EWeaponType::EWT_MAX:
		SectionName = FName("MAX");
		break;
	default:
		SectionName = FName("");
		break;
	}

	PlayMontage(ReloadMontage, SectionName);

	Combat->SetCombatState(ECombatState::ECS_Unoccupied);
}

void AMainCharacter::PlayThrowGrenadeMontage() const
{
	if (!GetMesh()) return;
	
	PlayMontage(ThrowGrenadeMontage, FName());
	if (Combat)
	{
		Combat->SetCombatState(ECombatState::ECS_Unoccupied);
	}
}

void AMainCharacter::UpdateMaterial(float CurveValue)
{
	if (!DynamicDissolveMatInst) return;
	DynamicDissolveMatInst->SetScalarParameterValue(FName("Dissolve"), CurveValue);
}

void AMainCharacter::StartDissolve()
{
	if (!TimelineComponent || !DissolveCurve || !GetMesh()) return;

	// Initialize the dynamic material.
	DynamicDissolveMatInst = UMaterialInstanceDynamic::Create(DissolveMatInst, this);
	if (!DynamicDissolveMatInst) return;

	GetMesh()->SetMaterial(0, DynamicDissolveMatInst);
	DynamicDissolveMatInst->SetScalarParameterValue(FName("Dissolve"), 0.55f);
	DynamicDissolveMatInst->SetScalarParameterValue(FName("Glow"), 200.f);

	// Initialize the timeline component.
	//DissolveTrack.BindDynamic(this, &ThisClass::UpdateMaterial);
	TimelineComponent->AddInterpFloat(DissolveCurve, DissolveTrack);
	TimelineComponent->Play();
}

void AMainCharacter::PlayElimBotEffect()
{
	const FVector SpawnLocation = GetActorLocation() + FVector(0.f, 0.f, 200.f);
	UGameplayStatics::SpawnEmitterAtLocation(this, ElimBot, SpawnLocation);
	UGameplayStatics::PlaySoundAtLocation(this, ElimBotSound, SpawnLocation, FRotator::ZeroRotator);
}






/**
 * @brief  Player Input
 * @param 
 */
void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ThisClass::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ThisClass::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ThisClass::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ThisClass::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ThisClass::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ThisClass::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ThisClass::FireButtonReleased);
	PlayerInputComponent->BindAction("SwitchFireMode", IE_Pressed, this, &ThisClass::SwitchFireModeButtonPressed);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ThisClass::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ThisClass::ThrowButtonPressed);
	PlayerInputComponent->BindAction("OpenMenu", IE_Pressed, this, &ThisClass::OpenMenuPressed);
	PlayerInputComponent->BindAxis("MoveForward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("TurnRight", this, &ThisClass::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ThisClass::LookUpAtRate);
}

void AMainCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMainCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AMainCharacter::TurnAtRate(float Value)
{
	AddControllerYawInput(Value * TurnRate * GetWorld()->GetDeltaSeconds());
}

void AMainCharacter::LookUpAtRate(float Value)
{
	AddControllerPitchInput(Value * TurnRate * GetWorld()->GetDeltaSeconds());
}

void AMainCharacter::EquipButtonPressed()
{
	if (Combat && OverlappingWeapon)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void AMainCharacter::Jump()
{
	if (bIsCrouched) UnCrouch();
	else Super::Jump();
}

void AMainCharacter::CrouchButtonPressed()
{
	if (GetCharacterMovement()->IsFalling()) return;
	
	if (bIsCrouched) UnCrouch();
	else Crouch();
}

void AMainCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AMainCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void AMainCharacter::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void AMainCharacter::FireButtonReleased()
{
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void AMainCharacter::SwitchFireModeButtonPressed()
{
	if (Combat)
	{
		Combat->SwitchFireModeButtonPressed();
	}
}

void AMainCharacter::ReloadButtonPressed()
{
	if (Combat)
	{
		Combat->Reload();
	}
}

void AMainCharacter::ThrowButtonPressed()
{
	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void AMainCharacter::OpenMenuPressed()
{
	if (!ShooterPlayerController)
	{
		ShooterPlayerController = Cast<AShooterPlayerController>(GetController());
		if (!ShooterPlayerController)
		{
			return;
		}
	}

	ShooterPlayerController->OpenMenu();
}

void AMainCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->MainCharacter = this;
	}
	if (Buff)
	{
		Buff->MainCharacter = this;
	}
}

void AMainCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMainCharacter, Health);
	DOREPLIFETIME(AMainCharacter, FireWeaponMontage);
	DOREPLIFETIME(AMainCharacter, HitReactMontage);
	DOREPLIFETIME(AMainCharacter, DeathHipMontage);
	DOREPLIFETIME(AMainCharacter, DeathIronMontage);
	DOREPLIFETIME(AMainCharacter, ReloadMontage);
	DOREPLIFETIME(AMainCharacter, ThrowGrenadeMontage);
	DOREPLIFETIME(AMainCharacter, MyOverheadWidget);
}

void AMainCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	// When the overlap ends, we should hide the PickupWidget's text. To make that, we need firstly 'ShowPickupWidget(false)' before we assign the
	// Weapon (Weapon == nullptr when overlap ends) to the OverlappingWeapon. Otherwise, we have no chance to change the PickupWidget's text.
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(false);
		}
	}
	
	OverlappingWeapon = Weapon;
	
	// Overlap begins, show the PickupWidget's text.
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void AMainCharacter::PlayFireMontage(bool bAiming) const
{
	if (!Combat || !Combat->EquippedWeapon) return;

	PlayMontage(FireWeaponMontage, bAiming ? FName("Aim_Fire") : FName("Hip_Fire"));
}

bool AMainCharacter::IsWeaponEquipped() const
{
	//if (Combat && GetPlayerState())
	//UE_LOG(LogTemp, Display, TEXT("Name %s, IsEquipped %s"), *GetPlayerState()->GetPlayerName(), Combat->EquippedWeapon ? TEXT("yes") : TEXT("no"));

	return (Combat && Combat->EquippedWeapon);
}

bool AMainCharacter::IsAiming() const
{
	return (Combat && Combat->bAiming);
}

bool AMainCharacter::IsFireButtonPressed() const
{
	return (Combat && Combat->bFireButtonPressed);
}

void AMainCharacter::OnRep_SetHealth(float PrevHealthValue)
{
	const bool IsHealthIncreased = PrevHealthValue < Health;
	HandleHealth(IsHealthIncreased);
}

void AMainCharacter::Multicast_PlayMontage_Implementation(UAnimMontage* MontageToPlay, const FName SectionName) const
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(MontageToPlay);
		if (!SectionName.IsNone())
			AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AMainCharacter::Server_PlayMontage_Implementation(UAnimMontage* MontageToPlay, const FName SectionName) const
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(MontageToPlay);
		if (!SectionName.IsNone())
			AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AMainCharacter::PlayMontage(UAnimMontage* MontageToPlay, const FName SectionName) const
{
	if (!MontageToPlay)
		return;

	if (GetLocalRole() < ROLE_Authority)
	{
		Server_PlayMontage(MontageToPlay, SectionName);
	}
	else
	{
		Multicast_PlayMontage(MontageToPlay, SectionName);
	}
}

void AMainCharacter::Server_SetHealth_Implementation(float HealthValue)
{
	float PrevHealthValue = Health;

	Health = HealthValue;
	OnRep_SetHealth(PrevHealthValue);

	UpdateOverHeadWidget();
}

void AMainCharacter::SetHealth(float HealthValue)
{
	if (HealthValue < 0.f || HealthValue > MaxHealth) return;
	
	Server_SetHealth(HealthValue);
}

void AMainCharacter::HandleHealth(bool IsHealthUp)
{
	SetHUDHealth();

	if (IsHealthUp) {}
	else
	{
		PlayHitReactMontage();
	}
}

void AMainCharacter::SetIsRespawned()
{
	// IsRespawned will restore its default value since the character is respawned.
	IsRespawned = true;
	HandleIsRespawned();
}

void AMainCharacter::HandleIsRespawned()
{
	if (!ShooterPlayerController)
	{
		ShooterPlayerController = Cast<AShooterPlayerController>(GetController());
		if (!ShooterPlayerController)
		{
			return;
		}
	}

	ShooterPlayerController->RefreshHUD();
}

ECombatState AMainCharacter::GetCombatState() const
{
	return Combat ? Combat->GetCombatState() : ECombatState::ECS_MAX;
}

void AMainCharacter::SetCombatState(ECombatState State)
{
	if (Combat)
	{
		Combat->SetCombatState(State);
	}
}


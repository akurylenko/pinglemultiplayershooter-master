 // Fill out your copyright notice in the Description page of Project Settings.


 #include "Character/ShooterAnimInstance.h"
 #include "Character/MainCharacter.h"
 #include "GameFramework/CharacterMovementComponent.h"
 #include "Kismet/KismetMathLibrary.h"
#include "Weapon/Weapon.h"

 void UShooterAnimInstance::NativeInitializeAnimation()
 {
 	Super::NativeInitializeAnimation();

 	MainCharacter = Cast<AMainCharacter>(TryGetPawnOwner());
 }

 void UShooterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
 {
 	Super::NativeUpdateAnimation(DeltaSeconds);

 	if (MainCharacter == nullptr)
 	{
 		MainCharacter = Cast<AMainCharacter>(TryGetPawnOwner());
 	}
 	if (MainCharacter == nullptr) return;

 	if (MainCharacter->IsLocallyControlled()) bIsLocallyControlled = true;
 	else bIsLocallyControlled = false;

 	FVector Velocity = MainCharacter->GetVelocity();
 	Velocity.Z = 0.f;
 	Speed = Velocity.Size();

 	bIsInAir = MainCharacter->GetCharacterMovement()->IsFalling();
 	bIsAccelerating = MainCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
 	bWeaponEquipped = MainCharacter->IsWeaponEquipped();
 	bIsCrouched = MainCharacter->bIsCrouched;
 	bAiming = MainCharacter->IsAiming();

 	// Offset yaw for strafing
 	// GetBaseAimRotation() is a global rotation, not local to the character.
 	const FRotator AimRotation = MainCharacter->GetBaseAimRotation();
 	// GetVelocity() is also a global rotation
 	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(MainCharacter->GetVelocity());
 	const FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
 	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaSeconds, 5.f);
 	YawOffset = DeltaRotation.Yaw;

 	LastFrameLean = NewFrameLean;
 	NewFrameLean = MainCharacter->GetActorRotation();
 	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(NewFrameLean, LastFrameLean);
 	const float TargetLean = Delta.Yaw / DeltaSeconds;
 	const float Interp = FMath::FInterpTo(Lean, TargetLean, DeltaSeconds, 5.f);
 	Lean = FMath::Clamp(Interp, -90.f, 90.f);

 	AO_Yaw = MainCharacter->GetAO_Yaw();
 	AO_Pitch = MainCharacter->GetAO_Pitch();

 	LeftHandTransform = MainCharacter->GetLeftHandTransform();

 	TurningInPlace = MainCharacter->GetTuringInPlace();

 	RightHandRotation = MainCharacter->GetRightHandRotation();

 	bUseFABRIK = MainCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
 	bUseAimOffset = MainCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
 	bTransformRightHand = MainCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
 }


// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShooterTypes/CombatState.h"
#include "ShooterTypes/TurningInPlace.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "Interface/InteractWithCrosshairsInterface.h"
#include "Weapon/Weapon.h"
#include "MainCharacter.generated.h"

DECLARE_DYNAMIC_DELEGATE(FOnThrowGrenadeMontageEnd);


class FOnTimelineFloat;
UCLASS()
class MULTIPLAYERSHOOTER_API AMainCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	AMainCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
	
protected:
	virtual void BeginPlay() override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void TurnAtRate(float Value);
	void LookUpAtRate(float Value);
	void EquipButtonPressed();
	virtual void Jump() override;
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void SwitchFireModeButtonPressed();
	void ReloadButtonPressed();
	void ThrowButtonPressed();

public:
	void SetOverlappingWeapon(class AWeapon* Weapon);
	void PlayFireMontage(bool bAiming) const;
	void PlayReloadMontage() const;
	void PlayThrowGrenadeMontage() const;
	void Eliminated();

	/* Display the sniper scope effect when aiming. */
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateOverHeadWidget();
	
private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, Category = Input)
	float TurnRate = 50.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(EditAnywhere, Category = Widget, ReplicatedUsing = OnRep_UpdateOverHeadWidget)
	class UOverheadWidget* MyOverheadWidget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UOverheadWidget> MyOverheadWidgetClass;

	UPROPERTY()
	class AWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class AShooterPlayerController* ShooterPlayerController;

	FRotator LastAimRotation;
	float AO_Yaw, Interp_AO_Yaw;
	float AO_Pitch;
	void AimOffset(float DeltaTime);	// Interpolation

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);	// Interpolation
	
	FTransform LeftHandTransform;
	void FABRIK_IK_LeftHand();
	
	FRotator RightHandRotation;
	void CorrectWeaponRotation(float DeltaTime);

	/**
	 *	Animation Montage
	 */
	UPROPERTY(EditAnywhere, Category = Combat, Replicated)
	UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat, Replicated)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat, Replicated)
	UAnimMontage* DeathHipMontage;

	UPROPERTY(EditAnywhere, Category = Combat, Replicated)
	UAnimMontage* DeathIronMontage;

	UPROPERTY(EditAnywhere, Category = Combat, Replicated)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat, Replicated)
	UAnimMontage* ThrowGrenadeMontage;

	/* Set a threshold between camera and the character to avoid blocking. */
	UPROPERTY(EditAnywhere, Category = Camera)
	float CameraThreshold = 200.f;

	/* Check if camera is too close with the character, if so, hide the character to avoid blocking the vision. */
	void HideCharacterIfClose();

	/**
	 *	Player status.
	 */
	UPROPERTY(EditAnywhere, Category = PlayerStats)
	float MaxHealth = 100.f;
	
	UPROPERTY(VisibleAnywhere, Category = PlayerStats, ReplicatedUsing = OnRep_SetHealth)
	float Health = 100.f;

	UPROPERTY(VisibleAnywhere, Category = PlayerStats)
	bool IsRespawned = false;

	FTimerHandle RespawnTimer;

	UPROPERTY(EditAnywhere, Category = PlayerStats)
	float TimerDelay = 3.f;
	
	void RespawnTimerFinished();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	void SetHUDHealth();
	void PlayHitReactMontage() const;
	void PlayDeathHipMontage() const;
	void PlayDeathIronMontage() const;
	

	/**
	 *	FX -- Elimination
	 */
	
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UTimelineComponent* TimelineComponent;
	
	UPROPERTY(EditAnywhere, Category = Elim)
	UCurveFloat* DissolveCurve;

	FOnTimelineFloat DissolveTrack;
	
	UFUNCTION()
	void UpdateMaterial(float CurveValue);

	void StartDissolve();

	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMatInst;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMatInst;
	
	UPROPERTY(EditAnywhere, Category = Elim)
	UParticleSystem* ElimBot;

	UPROPERTY(EditAnywhere, Category = Elim)
	USoundBase* ElimBotSound;

	void PlayElimBotEffect();
	

	/**
	 *	Attachment -- Grenade Mesh
	 */

	
	UPROPERTY(VisibleAnywhere, Category = Attachment)
	UStaticMeshComponent* GrenadeAttached;


	/**
	 *	Buff component
	 */


	UPROPERTY(VisibleAnywhere, Category = Buff)
	class UBuffComponent* Buff;

public:
	UFUNCTION(BlueprintCallable)
	bool IsWeaponEquipped() const;
	bool IsAiming() const;

	UFUNCTION(BlueprintCallable)
	UCombatComponent* GetCombat() const { return Combat; }
	
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE FTransform GetLeftHandTransform() const { return LeftHandTransform; }
	FORCEINLINE FRotator GetRightHandRotation() const { return RightHandRotation; }
	FORCEINLINE ETurningInPlace GetTuringInPlace() const { return TurningInPlace; }
	bool IsFireButtonPressed() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE float GetHealth() const { return Health; }
	UFUNCTION(BlueprintCallable)
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	void SetHealth(const float HealthValue);
	void HandleHealth(const bool IsHealthUp);
	FORCEINLINE void SetMaxHealth(const float MaxHealthValue) { MaxHealth = MaxHealthValue; }
	void SetIsRespawned();
	void HandleIsRespawned();
	ECombatState GetCombatState() const;
	void SetCombatState(ECombatState State);
	UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	UStaticMeshComponent* GetGrenadeAttached() const { return GrenadeAttached; }
	UBuffComponent* GetBuff() const { return Buff; }

protected:
	UFUNCTION()
	void OnRep_SetHealth(const float PrevHealthValue);

	UFUNCTION(Server, Reliable)
	void Server_SetHealth(const float HealthValue);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayMontage(UAnimMontage* MontageToPlay, const FName SectionName) const;

	UFUNCTION(Server, Reliable)
	void Server_PlayMontage(UAnimMontage* MontageToPlay, const FName SectionName) const;

	UFUNCTION()
	void PlayMontage(UAnimMontage* MontageToPlay, const FName SectionName) const;

	UFUNCTION()
	void OnRep_UpdateOverHeadWidget();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_UpdateOverHeadWidget();
};

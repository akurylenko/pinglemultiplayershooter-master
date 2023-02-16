// Fill out your copyright notice in the Description page of Project Settings.


#include "Notifies/AnimNotifyFootstep.h"
#include "Character/MainCharacter.h"
#include "Kismet/GameplayStatics.h"

// Optional Challenge: Crouch Footstep Sounds (Only the local player can hear)
void UAnimNotifyFootstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetAnimInstance())
	{
		APawn* PawnOwner = MeshComp->GetAnimInstance()->TryGetPawnOwner();
		if (const AMainCharacter* MainCharacter = Cast<AMainCharacter>(PawnOwner))
		{
			if(MainCharacter->IsLocallyControlled())
			{
				const FVector Location = MeshComp->GetSocketLocation(FName("Root"));
				UGameplayStatics::PlaySoundAtLocation(MeshComp->GetWorld(), Sound, Location, FRotator::ZeroRotator);
			}
		}
	}
}

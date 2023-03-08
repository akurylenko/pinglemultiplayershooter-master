// Fill out your copyright notice in the Description page of Project Settings.


#include "Notifies/AnimNotifyLaunchGrenade.h"
#include "Notifies/AnimNotifyReload.h"
#include "Character/MainCharacter.h"

void UAnimNotifyLaunchGrenade::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (MeshComp && MeshComp->GetAnimInstance())
	{
		APawn* PawnOwner = MeshComp->GetAnimInstance()->TryGetPawnOwner();
		AMainCharacter* MainCharacter = Cast<AMainCharacter>(PawnOwner);
		if (MainCharacter && MainCharacter->GetCombat())
		{
			MainCharacter->GetCombat()->LaunchGrenadeAnimNotify();
		}
	}
}

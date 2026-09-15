#include "Unit/UnitPresentation.h"
#include "Unit/Unit.h"
#include "Unit/UnitAnimSetDataAsset.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

UAnimInstance* UUnitPresentation::PlayMontageOn(AUnit* Owner, UAnimMontage* Montage) const
{
	if (!Owner || !Montage) { return nullptr; }

	USkeletalMeshComponent* MeshComp = Owner->GetMesh();
	UAnimInstance* AnimInst = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInst) { return nullptr; }

	return (AnimInst->Montage_Play(Montage) > 0.f) ? AnimInst : nullptr;
}

void UUnitPresentation::PlayAndNotifyWhenDone(AUnit* Owner, UAnimMontage* Montage)
{
	if (!Owner) { return; }   // 통보할 대상 자체가 없음

	UAnimInstance* AnimInst = PlayMontageOn(Owner, Montage);

	// 몽타주가 없거나 재생 실패 - 큐를 멈추지 말고 즉시 넘긴다
	if (!AnimInst)
	{
		Owner->NotifyMyPresentationFinished();
		return;
	}

	PendingOwner = Owner;

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UUnitPresentation::HandleMontageEnded);
	AnimInst->Montage_SetEndDelegate(EndDelegate, Montage);
}

void UUnitPresentation::HandleMontageEnded(UAnimMontage* /*Montage*/, bool /*bInterrupted*/)
{
	// 중단되어도 큐는 진행시킨다. 여기서 멈추면 게임이 정지한다.
	if (AUnit* Owner = PendingOwner.Get())
	{
		Owner->NotifyMyPresentationFinished();
	}
	PendingOwner.Reset();
}

void UUnitPresentation::PresentDeath_Implementation(AUnit* Owner)
{
	const UUnitAnimSetDataAsset* AnimSet = Owner ? Owner->GetAnimSet() : nullptr;
	PlayAndNotifyWhenDone(Owner, AnimSet ? AnimSet->DeathMontage : nullptr);
}

void UUnitPresentation::PresentRevive_Implementation(AUnit* Owner)
{
	const UUnitAnimSetDataAsset* AnimSet = Owner ? Owner->GetAnimSet() : nullptr;
	PlayAndNotifyWhenDone(Owner, AnimSet ? AnimSet->ReviveMontage : nullptr);
}

void UUnitPresentation::PresentHit_Implementation(AUnit* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("[UUnitPresentation] PresentHit: %s"), *GetNameSafe(Owner));
	// fire-and-forget. 큐를 점유하지 않으므로 완료 통보 없음.
	const UUnitAnimSetDataAsset* AnimSet = Owner ? Owner->GetAnimSet() : nullptr;
	PlayMontageOn(Owner, AnimSet ? AnimSet->HitMontage : nullptr);
}
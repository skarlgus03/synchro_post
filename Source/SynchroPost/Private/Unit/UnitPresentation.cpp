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
	if (!Owner) { return; }

	const UUnitAnimSetDataAsset* AnimSet = Owner->GetAnimSet();
	PlayMontageOn(Owner, AnimSet ? AnimSet->DeathMontage : nullptr);

	Owner->NotifyMyPresentationFinished();
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

void UUnitPresentation::PresentMoveSegment_Implementation(AUnit* Owner, const FIntPoint& From, const FIntPoint& To)
{
	if (!Owner) { return; }

	MoveElapsedTime = 0.f;

	MoveStartLocation = Owner->GetActorLocation();
	MoveEndLocation = Owner->GetStandLocation(To);
	MoveEndCoord = To;

	const FVector Direction(To.X - From.X, To.Y - From.Y, 0.f);
	MoveTargetRotation = FRotator(0.f, Direction.Rotation().Yaw, 0.f);
	
	const int32 NumTiles = FMath::Abs(To.X - From.X) + FMath::Abs(To.Y - From.Y);
	MoveDuration = NumTiles * SecondsPerTile;

	// 이동 시간이 0 이하이면 즉시 이동 완료 처리
	if (MoveDuration <= 0.f)
	{
		MoveDuration = 0.f;
		Owner->SnapToTile(MoveEndCoord);
		Owner->SetActorRotation(MoveTargetRotation);
		Owner->NotifyMyPresentationFinished();
		return;
	}
}

void UUnitPresentation::TickPresentation(AUnit* Owner, float DeltaTime)
{
	if (!Owner || !IsPresentingMove()) { return; }

	MoveElapsedTime += DeltaTime;
	const float Alpha = FMath::Clamp(MoveElapsedTime / MoveDuration, 0.f, 1.f);

	Owner->SetActorLocation(FMath::Lerp(MoveStartLocation, MoveEndLocation, Alpha));
	Owner->SetActorRotation(FMath::RInterpTo(Owner->GetActorRotation(), MoveTargetRotation, DeltaTime, RotationInterpSpeed));

	if (Alpha >= 1.f)
	{
		Owner->SnapToTile(MoveEndCoord);
		Owner->SetActorRotation(MoveTargetRotation);

		MoveDuration = 0.f;
		MoveElapsedTime = 0.f;

		Owner->NotifyMyPresentationFinished();
		return;
	}
}

bool UUnitPresentation::NeedsTick() const
{
	bool bNeedsTick = IsPresentingMove();

	return bNeedsTick;
}

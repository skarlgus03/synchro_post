#pragma once

#include "CoreMinimal.h"
#include "Unit/UnitPresentationBase.h"
#include "UnitPresentation.generated.h"

class UAnimInstance;
class UAnimMontage;

/*
* 기본 유닛 연출. AnimSet의 몽타주를 재생함
* 몽타주가 없으면 즉시 완료됨
*/
UCLASS()
class SYNCHROPOST_API UUnitPresentation : public UUnitPresentationBase
{
	GENERATED_BODY()
	
public:
	virtual void PresentDeath_Implementation(AUnit* Owner) override;
	virtual void PresentRevive_Implementation(AUnit* Owner) override;
	virtual void PresentHit_Implementation(AUnit* Owner) override;
	virtual void PresentMoveSegment_Implementation(AUnit* Owner, const FIntPoint& From, const FIntPoint& To) override;

	virtual void TickPresentation(AUnit* Owner, float DeltaTime) override;
	virtual bool IsPresentingMove() const override { return MoveDuration > 0.f; }
	virtual bool NeedsTick() const override;
	virtual float GetPresentationMoveSpeed() const override 
	{
		return IsPresentingMove()
			? FVector::Dist(MoveStartLocation, MoveEndLocation) / MoveDuration
			: 0.f;
	}
private:
	/** 몽타주를 재생하고, 성공하면 AnimInstance를 반환. 실패/없음이면 nullptr */
	UAnimInstance* PlayMontageOn(AUnit* Owner, UAnimMontage* Montage) const;

	/** 완료 통보가 필요한 연출(사망/부활)의 공통 처리 */
	void PlayAndNotifyWhenDone(AUnit* Owner, UAnimMontage* Montage);

	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	TWeakObjectPtr<AUnit> PendingOwner;

	FVector MoveStartLocation;
	FVector MoveEndLocation;
	FIntPoint MoveEndCoord;
	FRotator MoveTargetRotation;
	float MoveDuration = 0.f;
	float MoveElapsedTime = 0.f;
	float RotationInterpSpeed = 10.f;

	float SecondsPerTile = 0.25f; // 한 타일 이동에 걸리는 시간
};

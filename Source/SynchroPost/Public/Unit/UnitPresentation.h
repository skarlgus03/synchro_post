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
	virtual void PresentMoveSegment_Implementation(AUnit* Owner, const TArray<FIntPoint>& Waypoints) override;
	virtual void PresentFace_Implementation(AUnit* Owner, const FIntPoint& From, const FIntPoint& Toward) override;

	virtual void TickPresentation(AUnit* Owner, float DeltaTime) override;
	virtual bool IsPresentingMove() const override { return MoveDuration > 0.f; }
	virtual bool NeedsTick() const override;
	virtual float GetPresentationMoveSpeed() const override 
	{
		return (IsPresentingMove() && MoveCumulativeDist.Num() > 0)
			? MoveCumulativeDist.Last() / MoveDuration
			: 0.f;
	}
private:
	/** 몽타주를 재생하고, 성공하면 AnimInstance를 반환. 실패/없음이면 nullptr */
	UAnimInstance* PlayMontageOn(AUnit* Owner, UAnimMontage* Montage) const;

	/** 완료 통보가 필요한 연출(사망/부활)의 공통 처리 */
	void PlayAndNotifyWhenDone(AUnit* Owner, UAnimMontage* Montage);

	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	
	void TickFace(AUnit* Owner, float DeltaTime);

	// 사망 연출이 끝났음을 알린다.Notify함.
	void FinishDeath();

	/*Chaikin 모서리 꺾기, 시작점, 끝점은 고정. 깎은 점은 항상 원래 선분위에있음.*/
	static void SmoothPolyline(TArray<FVector>& Points, int32 Iterations);


	/*
	* 변수들
	*/

	TWeakObjectPtr<AUnit> PendingOwner;

	TArray<FVector> MovePoints; // 따라갈 꺾은 선 (월드 좌표)
	TArray<float> MoveCumulativeDist; // MovePoints[i] 까지의 누적 거리
	FRotator MoveFinalRotation; // 도착 시 맞출 방향.
	FIntPoint MoveEndCoord;
	FRotator FaceTargetRotation;
	bool bIsFacing = false;
	float MoveDuration = 0.f;
	float MoveElapsedTime = 0.f;
	float RotationInterpSpeed = 10.f;

	int32 SmoothIterations = 2; // 이동 경로 스무딩 반복 횟수

	float SecondsPerTile = 0.25f; // 한 타일 이동에 걸리는 시간

	FTimerHandle DeathTimerHandle;
};

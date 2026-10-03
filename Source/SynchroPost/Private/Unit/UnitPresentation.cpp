#include "Unit/UnitPresentation.h"
#include "Unit/Unit.h"
#include "Unit/UnitAnimSetDataAsset.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h" 
#include "Framework/GridManager.h"


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

void UUnitPresentation::TickFace(AUnit* Owner, float DeltaTime)
{
	const FRotator NewRotation = FMath::RInterpTo(Owner->GetActorRotation(), FaceTargetRotation, DeltaTime, RotationInterpSpeed);
	Owner->SetActorRotation(NewRotation);

	if (NewRotation.Equals(FaceTargetRotation, 1.f))
	{
		Owner->SetActorRotation(FaceTargetRotation);
		bIsFacing = false;
	}
}

void UUnitPresentation::FinishDeath()
{
	AUnit* Owner = PendingOwner.Get();
	PendingOwner.Reset();
	if (!Owner) { return; }


	// 시체를 치움. 부활이 있으므로 객체를 파괴하지 않고, 숨기기만 함.
	Owner->SetActorHiddenInGame(true);
	Owner->SetActorEnableCollision(false);

	Owner->NotifyMyPresentationFinished();
}

void UUnitPresentation::SmoothPolyline(TArray<FVector>& Points, int32 Iterations)
{
	for (int32 Iter = 0; Iter < Iterations; ++Iter)
	{
		if (Points.Num() < 3)
		{
			return; // 점 2개(직선)는 깎을 모서리가 없다
		}

		TArray<FVector> Out;
		Out.Reserve(Points.Num() * 2);
		Out.Add(Points[0]); // 시작점 고정

		for (int32 i = 0; i < Points.Num() - 1; ++i)
		{
			const FVector& A = Points[i];
			const FVector& B = Points[i + 1];
			Out.Add(FMath::Lerp(A, B, 0.25f)); // 선분의 1/4 지점
			Out.Add(FMath::Lerp(A, B, 0.75f)); // 선분의 3/4 지점
		}

		Out.Add(Points.Last()); // 끝점 고정
		Points = MoveTemp(Out);
	}
}

void UUnitPresentation::PresentDeath_Implementation(AUnit* Owner)
{
	if (!Owner) { return; }

	PendingOwner = Owner;

	const UUnitAnimSetDataAsset* AnimSet = Owner->GetAnimSet();
	UAnimMontage* Montage = AnimSet ? AnimSet->DeathMontage : nullptr;

	const float Duration = (Montage && PlayMontageOn(Owner, Montage))
		? Montage->GetPlayLength()
		: 0.f;

	if (Duration <= 0.f)
	{
		FinishDeath();
		return;
	}

	Owner->GetWorldTimerManager().SetTimer(DeathTimerHandle, this, &UUnitPresentation::FinishDeath, Duration, false);

}

void UUnitPresentation::PresentRevive_Implementation(AUnit* Owner)
{
	if (Owner)
	{
		Owner->SetActorHiddenInGame(false);
		Owner->SetActorEnableCollision(true);
	}
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

void UUnitPresentation::PresentMoveSegment_Implementation(AUnit* Owner, const TArray<FIntPoint>& Waypoints)
{
	if (!Owner)
	{
		return;
	}
	bIsFacing = false; // 이동이 방향을 정한다.

	if (Waypoints.Num() < 2)
	{
		// 이동할 좌표가 없거나, 한 칸만 있으면 즉시 이동 완료 처리
		if (Waypoints.Num() == 1)
		{
			Owner->SnapToTile(Waypoints[0]);
		}
		Owner->NotifyMyPresentationFinished();
		return;
	}

	// 마지막 칸에서의 회전은 마지막 두 칸을 보고 결정한다.
	MoveEndCoord = Waypoints.Last();
	CalcFacingRotation(Waypoints[Waypoints.Num() - 2], MoveEndCoord, MoveFinalRotation);
	
	// 점 목록 만들기. (1단계) Waypoints를 따라가는 꺾은 선을 만든다.
	MovePoints.Reset();
	MovePoints.Add(Owner->GetActorLocation());
	for (int32 i = 1; i < Waypoints.Num(); ++i)
	{
		MovePoints.Add(Owner->GetStandLocation(Waypoints[i]));
	}

	// (2단계) 여기서 MovePoints를 스무딩한다
	SmoothPolyline(MovePoints, SmoothIterations);


	// (3단계) MovePoints를 따라가는 누적 거리 배열을 만든다.
	MoveCumulativeDist.Reset();
	MoveCumulativeDist.Add(0.f);
	for (int32 i = 1; i < MovePoints.Num(); ++i)
	{
		MoveCumulativeDist.Add(MoveCumulativeDist.Last() + FVector::Dist(MovePoints[i - 1], MovePoints[i]));
	}

	

	// (4단계) 이동 시간 계산.
	const int32 NumTiles = Waypoints.Num() - 1;
	const float TotalLength = MoveCumulativeDist.Last();

	const UGridManager* Grid = Owner->GetWorld() ? Owner->GetWorld()->GetSubsystem<UGridManager>() : nullptr;
	const float TileSize = Grid ? Grid->GetTileSize() : 0.f;

	MoveDuration = NumTiles * SecondsPerTile;                                   // A: 시간 고정
	// MoveDuration = (TileSize > 0.f) ? TotalLength / (TileSize / SecondsPerTile)  // B: 속도 고정
	//                                 : NumTiles * SecondsPerTile;
	MoveElapsedTime = 0.f;


	// 이동 연출이 없거나, 이동 거리가 거의 없으면 즉시 스냅하고 종료
	if (MoveDuration <= 0.f || MoveCumulativeDist.Last() <= KINDA_SMALL_NUMBER)
	{
		MoveDuration = 0.f;
		Owner->SnapToTile(MoveEndCoord);
		Owner->SetActorRotation(MoveFinalRotation);
		Owner->NotifyMyPresentationFinished();
	}
}

void UUnitPresentation::PresentFace_Implementation(AUnit* Owner, const FIntPoint& From, const FIntPoint& Toward)
{
	FRotator Target;
	if (!Owner || !CalcFacingRotation(From, Toward, Target))
	{
		return;
	}
	FaceTargetRotation = Target;
	bIsFacing = true;
}

void UUnitPresentation::TickPresentation(AUnit* Owner, float DeltaTime)
{
	if (!Owner)
	{
		return;
	}

	// 이동 연출이 없으면 회전만 처리
	if (!IsPresentingMove())
	{
		if (bIsFacing)
		{
			TickFace(Owner, DeltaTime);
		}
		return;
	}

	// 경과 시간을 더하고 진행률을 구해 출발부터 현재까지 이동한 거리 계산
	MoveElapsedTime += DeltaTime;
	const float Alpha = FMath::Clamp(MoveElapsedTime / MoveDuration, 0.f, 1.f);
	const float Distance = Alpha * MoveCumulativeDist.Last();

	// Distance가 들어 있는 선분 [Seg-1, Seg] 찾기
	int32 Seg = 1;
	while (Seg < MoveCumulativeDist.Num() - 1 && MoveCumulativeDist[Seg] < Distance)
	{
		++Seg;
	}
	
	/*
	* 이 선분이 몇 cm에서 시작하는지 , 길이가 얼마인지 구한 후
	* 선분 안에서의 비율 T를 계산해 Lerp로 위치를 구한다.
	* SegLength 가 0인경우엔 0으로 나누지 않게 1로 처리함.
	*/
	const float SegStart = MoveCumulativeDist[Seg - 1];
	const float SegLength = MoveCumulativeDist[Seg] - SegStart;
	const float T = (SegLength > KINDA_SMALL_NUMBER) ? (Distance - SegStart) / SegLength : 1.f;

	Owner->SetActorLocation(FMath::Lerp(MovePoints[Seg - 1], MovePoints[Seg], T));

	/*
	* 선분의 진행 방향을 따라 회전
	* 지금 선분이 향하는 방향을 Yaw로 구하고 
	* 현재 회전에서 그쪽으로 RInterpTo로 회전시킨다.
	*/
	const FVector Dir = MovePoints[Seg] - MovePoints[Seg - 1];
	if (!Dir.IsNearlyZero())
	{
		const FRotator Facing(0.f, Dir.Rotation().Yaw, 0.f);
		Owner->SetActorRotation(FMath::RInterpTo(Owner->GetActorRotation(), Facing, DeltaTime, RotationInterpSpeed));
	}

	/*
	* 마지막 칸에 정확히 스냅하고, 회전도 맞춘 후 연출 종료 통보
	*/
	if (Alpha >= 1.f)
	{
		Owner->SnapToTile(MoveEndCoord);
		Owner->SetActorRotation(MoveFinalRotation);

		MoveDuration = 0.f;
		MoveElapsedTime = 0.f;

		Owner->NotifyMyPresentationFinished(); // 마지막 줄
		return;
	}
}

bool UUnitPresentation::NeedsTick() const
{
	bool bNeedsTick = IsPresentingMove() || bIsFacing;

	return bNeedsTick;
}

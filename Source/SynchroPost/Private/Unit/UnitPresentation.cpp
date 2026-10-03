#include "Unit/UnitPresentation.h"
#include "Unit/Unit.h"
#include "Unit/UnitAnimSetDataAsset.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h" 
#include "Framework/GridManager.h"
#include "Math/GridMath.h"

#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDebugMovePath(
	TEXT("sp.MovePath.Debug"),
	0,
	TEXT("이동 경로 시각화. 0: 끔 / 1: 원래 경로(빨강) + 실제 경로(초록) / 2: + 경로 칸(노랑), 실 당기기(파랑)"),
	ECVF_Cheat);


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

void UUnitPresentation::RoundCorners(TArray<FVector>& Points, float MaxRadius, int32 SamplesPerCorner)
{
	if (Points.Num() < 3 || MaxRadius <= 0.f || SamplesPerCorner < 1)
	{
		return;
	}

	TArray<FVector> Out;
	Out.Add(Points[0]);

	for (int32 i = 1; i < Points.Num() - 1; ++i)
	{
		const FVector& Prev = Points[i - 1];
		const FVector& Corner = Points[i];
		const FVector& Next = Points[i + 1];

		// 이웃 모서리의 곡선과 겹치지 않게 선분 길이의 절반까지만
		const float Radius = FMath::Min3(MaxRadius,
			static_cast<float>(FVector::Dist(Prev, Corner)) * 0.5f,
			static_cast<float>(FVector::Dist(Corner, Next)) * 0.5f);

		if (Radius <= KINDA_SMALL_NUMBER)
		{
			Out.Add(Corner);
			continue;
		}

		const FVector In = Corner + (Prev - Corner).GetSafeNormal() * Radius;
		const FVector Out_ = Corner + (Next - Corner).GetSafeNormal() * Radius;

		// 2차 베지어: In → (조절점 Corner) → Out_. 세 점이 모두 모서리 칸 안 → 곡선도 칸 안
		for (int32 s = 0; s <= SamplesPerCorner; ++s)
		{
			const float T = static_cast<float>(s) / SamplesPerCorner;
			Out.Add(FMath::Lerp(FMath::Lerp(In, Corner, T), FMath::Lerp(Corner, Out_, T), T));
		}
	}

	Out.Add(Points.Last());
	Points = MoveTemp(Out);
}

FVector UUnitPresentation::SamplePath(float Distance) const
{
	int32 Seg = 1;
	while (Seg < MoveCumulativeDist.Num() - 1 && MoveCumulativeDist[Seg] < Distance)
	{
		++Seg;
	}
	const float SegStart = MoveCumulativeDist[Seg - 1];
	const float SegLength = MoveCumulativeDist[Seg] - SegStart;
	const float T = (SegLength > KINDA_SMALL_NUMBER) ? (Distance - SegStart) / SegLength : 1.f;
	return FMath::Lerp(MovePoints[Seg - 1], MovePoints[Seg], FMath::Clamp(T, 0.f, 1.f));
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
	bIsFacing = false; // 이동이 방향을 정한다

	if (Waypoints.Num() < 2)
	{
		if (Waypoints.Num() == 1)
		{
			Owner->SnapToTile(Waypoints[0]);
		}
		Owner->NotifyMyPresentationFinished();
		return;
	}

	// ── 도착 정보 ────────────────────────────────────────────
	// 도착 방향은 격자의 마지막 두 칸으로 정확한 4방향
	MoveEndCoord = Waypoints.Last();
	CalcFacingRotation(Waypoints[Waypoints.Num() - 2], MoveEndCoord, MoveFinalRotation);

	const UGridManager* Grid = Owner->GetWorld() ? Owner->GetWorld()->GetSubsystem<UGridManager>() : nullptr;
	const float TileSize = Grid ? Grid->GetTileSize() : 0.f;

	// ── 1) 격자에서: 실 당기기 → 일직선 합치기 ──────────────
// 꼭짓점을 스칠 때 반대편 칸이 빈 바닥이어야 통과 (장애물·다른 유닛에 몸이 파고들지 않게)
	auto IsOpenTile = [Grid](const FIntPoint& Coord)
		{
			return Grid && Grid->IsWalkable(Coord) && Grid->GetUnitAt(Coord) == nullptr;
		};
	const TArray<FIntPoint> Corners = GridMath::MergeCollinear(
		GridMath::PullString(Waypoints, PullMaxSkip, IsOpenTile));
	const TSet<FIntPoint> PathTiles(Waypoints);

	// ── 2) 월드 좌표로 ───────────────────────────────────────
	// 첫 점은 실제 현재 위치. 꼭짓점을 스치는 대각선은 계단 띠 가운데로 옮긴다
	MovePoints.Reset();
	MovePoints.Add(Owner->GetActorLocation());
	for (int32 i = 1; i < Corners.Num(); ++i)
	{
		const FIntPoint& A = Corners[i - 1];
		const FIntPoint& B = Corners[i];

		FVector2D Offset;
		if (TileSize > 0.f && GridMath::GetDiagonalClearanceOffset(A, B, PathTiles, Offset))
		{
			const FVector Shift(Offset.X * TileSize, Offset.Y * TileSize, 0.f);
			const FVector Dir(FMath::Sign(B.X - A.X) * TileSize, FMath::Sign(B.Y - A.Y) * TileSize, 0.f);
			MovePoints.Add(Owner->GetStandLocation(A) + Dir * 0.5f + Shift); // 반 칸 앞에서 평행선에 합류
			MovePoints.Add(Owner->GetStandLocation(B) - Dir * 0.5f + Shift); // 반 칸 전에 평행선에서 이탈
		}
		MovePoints.Add(Owner->GetStandLocation(B));
	}

	// ── 3) 꺾이는 곳을 그 칸 안에서 둥글게 ──────────────────
	RoundCorners(MovePoints, TileSize * CornerRadiusRatio, CornerSamples);

	// ── 4) 누적 거리 표 (점 목록이 확정된 뒤에) ─────────────
	MoveCumulativeDist.Reset();
	MoveCumulativeDist.Add(0.f);
	for (int32 i = 1; i < MovePoints.Num(); ++i)
	{
		MoveCumulativeDist.Add(MoveCumulativeDist.Last() + static_cast<float>(FVector::Dist(MovePoints[i - 1], MovePoints[i])));
	}
	const float TotalLength = MoveCumulativeDist.Last();

	// ── 5) 소요 시간 ─────────────────────────────────────────
	const int32 NumTiles = Waypoints.Num() - 1;
	MoveDuration = NumTiles * SecondsPerTile;                                       // A: 시간 고정
	// MoveDuration = (TileSize > 0.f) ? TotalLength / (TileSize / SecondsPerTile)  // B: 속도 고정
	//                                 : NumTiles * SecondsPerTile;
	MoveElapsedTime = 0.f;


	// 디버그하기
	DrawDebugMovePath(Owner, Waypoints, Corners, TileSize);

	// ── 6) 갈 거리가 없으면 즉시 완료 ────────────────────────
	if (MoveDuration <= 0.f || TotalLength <= KINDA_SMALL_NUMBER)
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

	const float TotalLength = MoveCumulativeDist.Last();

	// 위치: 지금 거리
	const FVector Position = SamplePath(Distance);
	Owner->SetActorLocation(Position);

	// 회전: 경로상 조금 앞을 바라본다. 끝에 다다르면 도착 방향
	const FVector Ahead = SamplePath(FMath::Min(Distance + FacingLookAhead, TotalLength));
	const FVector ToAhead = Ahead - Position;
	const FRotator Facing = (ToAhead.SizeSquared2D() > 1.f)
		? FRotator(0.f, ToAhead.Rotation().Yaw, 0.f)
		: MoveFinalRotation;
	Owner->SetActorRotation(FMath::RInterpTo(Owner->GetActorRotation(), Facing, DeltaTime, RotationInterpSpeed));

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


void UUnitPresentation::DrawDebugMovePath(const AUnit* Owner, const TArray<FIntPoint>& Waypoints,
	const TArray<FIntPoint>& Corners, float TileSize) const
{
#if ENABLE_DRAW_DEBUG
	const int32 Level = CVarDebugMovePath.GetValueOnGameThread();
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (Level <= 0 || !World)
	{
		return;
	}

	const float LifeTime = MoveDuration + 1.5f; // 걷는 동안 + 도착 후 잠깐
	auto Lift = [](const FVector& P, float Z) { return P + FVector(0.f, 0.f, Z); }; // 선끼리 겹치지 않게 높이만 다르게

	// 경로 칸 바닥 (노랑)
	if (Level >= 2)
	{
		const FVector Half(TileSize * 0.45f, TileSize * 0.45f, 2.f);
		for (const FIntPoint& Tile : Waypoints)
		{
			DrawDebugBox(World, Lift(Owner->GetStandLocation(Tile), 2.f), Half, FColor::Yellow, false, LifeTime, 0, 2.f);
		}
	}

	// 원래 경로: 칸 중심을 잇는 꺾은선 (빨강)
	for (int32 i = 1; i < Waypoints.Num(); ++i)
	{
		DrawDebugLine(World,
			Lift(Owner->GetStandLocation(Waypoints[i - 1]), 5.f),
			Lift(Owner->GetStandLocation(Waypoints[i]), 5.f),
			FColor::Red, false, LifeTime, 0, 4.f);
	}

	// 실 당기기 결과 (파랑)
	if (Level >= 2)
	{
		for (int32 i = 1; i < Corners.Num(); ++i)
		{
			DrawDebugLine(World,
				Lift(Owner->GetStandLocation(Corners[i - 1]), 10.f),
				Lift(Owner->GetStandLocation(Corners[i]), 10.f),
				FColor::Blue, false, LifeTime, 0, 4.f);
		}
	}

	// 실제로 걷는 최종 경로 (초록)
	for (int32 i = 1; i < MovePoints.Num(); ++i)
	{
		DrawDebugLine(World, Lift(MovePoints[i - 1], 15.f), Lift(MovePoints[i], 15.f),
			FColor::Green, false, LifeTime, 0, 5.f);
	}
#endif
}
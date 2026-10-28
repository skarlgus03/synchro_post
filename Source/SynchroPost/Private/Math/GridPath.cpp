#include "Math/GridPath.h"
#include "Math/GridMath.h"
#include "Framework/GridManager.h"

namespace GridPathPrivate
{
	constexpr int32 PullMaxSkip = 2;          // 한 번에 최대 n칸까지만 직선으로
	constexpr float CornerRadiusRatio = 0.4f; // 타일 크기 대비. 0.5 미만이어야 안전
	constexpr int32 CornerSamples = 6;

	/** 꺾이는 점마다 반지름 이내에서 2차 베지어로 둥글게. 반지름 < 반 칸이면 곡선이 그 칸을 벗어나지 않는다 */
	void RoundCorners(TArray<FVector>& Points, float MaxRadius, int32 SamplesPerCorner)
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
}

TArray<FVector> GridPath::BuildSmoothPath(
	const UGridManager& Grid,
	const TArray<FIntPoint>& Waypoints,
	TFunctionRef<FVector(const FIntPoint&)> ToWorld,
	const TOptional<FVector>& StartLocation,
	TArray<FIntPoint>* OutCorners)
{
	using namespace GridPathPrivate;

	TArray<FVector> Points;
	if (Waypoints.Num() < 2)
	{
		return Points;
	}

	const float TileSize = Grid.GetTileSize();

	// ── 1) 격자에서: 실 당기기 → 일직선 합치기 ──────────────
	// 꼭짓점을 스칠 때 반대편 칸이 빈 바닥이어야 통과 (장애물·다른 유닛에 몸이 파고들지 않게)
	auto IsOpenTile = [&Grid](const FIntPoint& Coord)
		{
			return Grid.IsWalkable(Coord) && Grid.GetUnitAt(Coord) == nullptr;
		};
	const TArray<FIntPoint> Corners = GridMath::MergeCollinear(
		GridMath::PullString(Waypoints, PullMaxSkip, IsOpenTile));
	const TSet<FIntPoint> PathTiles(Waypoints);

	// ── 2) 월드 좌표로 ───────────────────────────────────────
	// 꼭짓점을 스치는 대각선은 계단 띠 가운데로 옮긴다
	Points.Add(StartLocation.IsSet() ? StartLocation.GetValue() : ToWorld(Corners[0]));
	for (int32 i = 1; i < Corners.Num(); ++i)
	{
		const FIntPoint& A = Corners[i - 1];
		const FIntPoint& B = Corners[i];

		FVector2D Offset;
		if (TileSize > 0.f && GridMath::GetDiagonalClearanceOffset(A, B, PathTiles, Offset))
		{
			const FVector Shift(Offset.X * TileSize, Offset.Y * TileSize, 0.f);
			const FVector Dir(FMath::Sign(B.X - A.X) * TileSize, FMath::Sign(B.Y - A.Y) * TileSize, 0.f);
			Points.Add(ToWorld(A) + Dir * 0.5f + Shift); // 반 칸 앞에서 평행선에 합류
			Points.Add(ToWorld(B) - Dir * 0.5f + Shift); // 반 칸 전에 평행선에서 이탈
		}
		Points.Add(ToWorld(B));
	}

	// ── 3) 꺾이는 곳을 그 칸 안에서 둥글게 ──────────────────
	RoundCorners(Points, TileSize * CornerRadiusRatio, CornerSamples);

	if (OutCorners)
	{
		*OutCorners = Corners;
	}
	return Points;
}
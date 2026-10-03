#pragma once

#include "CoreMinimal.h"
#include "Algo/Reverse.h"

namespace GridMath
{
	FORCEINLINE FIntPoint RotateOffset (const FIntPoint& Offset, const FIntPoint& Direction)
	{
		if (Direction == FIntPoint(0, 1)) // North
		{
			return Offset;
		}
		else if (Direction == FIntPoint(1, 0)) // East
		{
			return FIntPoint(Offset.Y, -Offset.X);
		}
		else if (Direction == FIntPoint(0, -1)) // South
		{
			return FIntPoint(-Offset.X, -Offset.Y);
		}
		else if (Direction == FIntPoint(-1, 0)) // West
		{
			return FIntPoint(-Offset.Y, Offset.X);
		}
		return Offset; // Default case, no rotation
	}

	FORCEINLINE FIntPoint ToCardinalDirection(const FIntPoint& Delta)
	{
		if (FMath::Abs(Delta.X) > FMath::Abs(Delta.Y))
		{
			return FIntPoint(FMath::Sign(Delta.X), 0); 
		}
		else
		{
			return FIntPoint(0, FMath::Sign(Delta.Y)); 
		}
	}


	/*
	 * 두 칸 중심을 잇는 직선이 지나는 칸이 전부 Allowed 안에 있는가.
	 * 직선이 칸 꼭짓점을 정확히 지나면, 양옆 칸 중 하나라도 Allowed면 통과로 본다.
	 * (1칸씩 번갈아 꺾는 계단을 완전한 대각선으로 펴기 위함. 벽 모서리를 몸이 살짝 스칠 수 있음)
	 */
	inline bool IsLineInsideTiles(const FIntPoint& A, const FIntPoint& B, const TSet<FIntPoint>& Allowed,
		TFunctionRef<bool(const FIntPoint&)> IsOpenTile)
	{
		const int32 Dx = B.X - A.X;
		const int32 Dy = B.Y - A.Y;
		const int32 Nx = FMath::Abs(Dx);
		const int32 Ny = FMath::Abs(Dy);
		const int32 Sx = (Dx > 0) ? 1 : -1;
		const int32 Sy = (Dy > 0) ? 1 : -1;

		FIntPoint P = A;
		if (!Allowed.Contains(P))
		{
			return false;
		}

		int32 Ix = 0;
		int32 Iy = 0;
		while (Ix < Nx || Iy < Ny)
		{
			// 다음에 넘는 칸 경계가 세로선인지 가로선인지 비교
			const int32 Decision = (1 + 2 * Ix) * Ny - (1 + 2 * Iy) * Nx;
			if (Decision == 0)
			{
				// 꼭짓점 통과: 양옆이 경로 칸이거나, 한쪽이 경로 칸이고 반대편이 빈 바닥이면 통과
				const FIntPoint SideX(P.X + Sx, P.Y);
				const FIntPoint SideY(P.X, P.Y + Sy);
				const bool bPathX = Allowed.Contains(SideX);
				const bool bPathY = Allowed.Contains(SideY);
				const bool bPass = (bPathX && bPathY)
					|| (bPathX && IsOpenTile(SideY))
					|| (bPathY && IsOpenTile(SideX));
				if (!bPass)
				{
					return false;
				}
				P.X += Sx; P.Y += Sy; ++Ix; ++Iy;
			}
			else if (Decision < 0)
			{
				P.X += Sx; ++Ix;
			}
			else
			{
				P.Y += Sy; ++Iy;
			}

			if (!Allowed.Contains(P))
			{
				return false;
			}
		}
		return true;
	}

	/**
	 * 실 당기기: 경유 칸 중 "직선으로 건너뛸 수 있는" 중간 칸을 버리고 꺾이는 칸만 남긴다.
	 * 직선은 항상 경유 칸들 안에서만 지나간다.
	 */
	inline TArray<FIntPoint> PullString(const TArray<FIntPoint>& Waypoints, int32 MaxSkip,
		TFunctionRef<bool(const FIntPoint&)> IsOpenTile)
	{
		if (Waypoints.Num() <= 2)
		{
			return Waypoints;
		}
		// 방향과 상관없이 같은 모양이 나오도록, 좌표가 작은 쪽 끝에서부터 당긴다
		const FIntPoint& First = Waypoints[0];
		const FIntPoint& Last = Waypoints.Last();
		const bool bReverse = (Last.X < First.X) || (Last.X == First.X && Last.Y < First.Y);
		if (bReverse)
		{
			TArray<FIntPoint> Reversed = Waypoints;
			Algo::Reverse(Reversed);
			TArray<FIntPoint> Result = PullString(Reversed, MaxSkip, IsOpenTile); // 정방향으로 당기고
			Algo::Reverse(Result);                                                // 다시 뒤집는다
			return Result;
		}

		const TSet<FIntPoint> Allowed(Waypoints);
		TArray<FIntPoint> Result;
		Result.Add(Waypoints[0]);

		int32 Anchor = 0;
		while (Anchor < Waypoints.Num() - 1)
		{
			int32 Farthest = Anchor + 1;
			// MaxSkip 칸 앞부터 거꾸로 확인
			const int32 SearchFrom = (MaxSkip >= Waypoints.Num())
				? Waypoints.Num() - 1
				: FMath::Min(Waypoints.Num() - 1, Anchor + MaxSkip);
			for (int32 j = SearchFrom; j > Anchor + 1; --j)
			{
				if (IsLineInsideTiles(Waypoints[Anchor], Waypoints[j], Allowed, IsOpenTile))
				{
					Farthest = j;
					break;
				}
			}
			Result.Add(Waypoints[Farthest]);
			Anchor = Farthest;
		}
		return Result;
	}

	/** 일직선으로 이어진 중간 점을 지운다. 짧은 대각선 조각들이 긴 대각선 하나로 합쳐진다 */
	inline TArray<FIntPoint> MergeCollinear(const TArray<FIntPoint>& Points)
	{
		if (Points.Num() <= 2)
		{
			return Points;
		}
		TArray<FIntPoint> Out;
		Out.Add(Points[0]);
		for (int32 i = 1; i < Points.Num() - 1; ++i)
		{
			const FIntPoint D1 = Points[i] - Out.Last();
			const FIntPoint D2 = Points[i + 1] - Points[i];
			const bool bSameLine = (D1.X * D2.Y - D1.Y * D2.X) == 0   // 평행
				&& (D1.X * D2.X + D1.Y * D2.Y) > 0;   // 같은 방향
			if (!bSameLine)
			{
				Out.Add(Points[i]);
			}
		}
		Out.Add(Points.Last());
		return Out;
	}

	/**
	 * A→B가 45도 대각선이고 지나는 꼭짓점마다 경로 칸이 늘 같은 쪽에 있으면,
	 * 그쪽으로 1/4칸 옮길 오프셋(칸 단위)을 돌려준다. 옮긴 평행선은 계단 띠의 한가운데를 지난다.
	 */
	inline bool GetDiagonalClearanceOffset(const FIntPoint& A, const FIntPoint& B, const TSet<FIntPoint>& PathTiles, FVector2D& OutOffset)
	{
		const int32 Dx = B.X - A.X;
		const int32 Dy = B.Y - A.Y;
		if (Dx == 0 || FMath::Abs(Dx) != FMath::Abs(Dy))
		{
			return false; // 45도 대각선만 처리
		}
		const int32 Sx = FMath::Sign(Dx);
		const int32 Sy = FMath::Sign(Dy);

		bool bAllXSide = true;
		bool bAllYSide = true;
		for (int32 k = 0; k < FMath::Abs(Dx); ++k)
		{
			const FIntPoint P(A.X + Sx * k, A.Y + Sy * k);
			bAllXSide &= PathTiles.Contains(FIntPoint(P.X + Sx, P.Y));
			bAllYSide &= PathTiles.Contains(FIntPoint(P.X, P.Y + Sy));
		}

		// 양쪽 다 경로 칸이면 꼭짓점에 닿을 일이 없고, 번갈아 나오면 한쪽으로 옮길 수 없다
		if (bAllXSide == bAllYSide)
		{
			return false;
		}

		OutOffset = bAllXSide
			? FVector2D(0.25f * Sx, -0.25f * Sy)   // X쪽 경로 칸 방향으로
			: FVector2D(-0.25f * Sx, 0.25f * Sy);  // Y쪽 경로 칸 방향으로
		return true;
	}
}
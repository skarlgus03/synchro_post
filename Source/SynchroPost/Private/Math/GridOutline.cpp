#include "Math/GridOutline.h"

#include "DrawDebugHelpers.h"
#include "SynchroPost.h"
#include "Engine/World.h"
#include "Framework/GridManager.h"
#include "HAL/IConsoleManager.h"


static TAutoConsoleVariable<int32> CVarDebugGridOutline(
	TEXT("sp.GridOutline.Debug"),
	0,
	TEXT("범위 외곽선 시각화, 0: 끔/ 1: 켬 , (바깥 초록, 구멍 주황, 화살표 = 진행방향"),
	ECVF_Cheat);

namespace GridOutlinePrivate 
{
	struct FBoundaryEdge
	{
		FIntPoint From;
		FIntPoint To;
	};

	FIntPoint DirOf(const FBoundaryEdge& Edge)
	{
		return Edge.To - Edge.From;
	}

	// 진행 방향 D에서 C로 꺾을 때의 우선순위 : 왼쪽 0, 직진 1, 오른쪽 2
	int32 TurnRank(const FIntPoint& D, const FIntPoint& C)
	{
		int32 Cross = D.X * C.Y - D.Y * C.X;
		if (Cross > 0) return 0; // 왼쪽
		if (Cross == 0) return 1; // 직진
		return 2; // 오른쪽
	}
}

TArray<TArray<FIntPoint>> GridOutline::BuildLoops(const TArray<FIntPoint>& Tiles)
{
	using namespace GridOutlinePrivate;

	const TSet<FIntPoint> Set(Tiles);

	// 같은 입력이면 같은 결과가 나오도록 정렬된 순서로 돈다.
	TArray<FIntPoint> Sorted = Set.Array();
	// x좌표 우선, 그다음 y좌표 비교
	Sorted.Sort([](const FIntPoint& A, const FIntPoint& B) {
		if (A.X != B.X) return A.X < B.X;
		return A.Y < B.Y;
		});

	// 1. 각 타일의 경계 변 찾기: 타일 하나의 4변중에, 인접한 타일이 집합 안에 존재 한다면 그 변은 제외하고, 인접한 타일이 없다면 그 변을 포함한다.
	TArray<FBoundaryEdge> Edges;
	for (const FIntPoint& T : Sorted)
	{
		const int32 X = T.X;
		const int32 Y = T.Y;
		if (!Set.Contains(FIntPoint(X, Y - 1))) { Edges.Add({ FIntPoint(X, Y),         FIntPoint(X + 1, Y) }); }      // 아래
		if (!Set.Contains(FIntPoint(X + 1, Y))) { Edges.Add({ FIntPoint(X + 1, Y),     FIntPoint(X + 1, Y + 1) }); }  // 오른쪽
		if (!Set.Contains(FIntPoint(X, Y + 1))) { Edges.Add({ FIntPoint(X + 1, Y + 1), FIntPoint(X, Y + 1) }); }      // 위
		if (!Set.Contains(FIntPoint(X - 1, Y))) { Edges.Add({ FIntPoint(X, Y + 1),     FIntPoint(X, Y) }); }          // 왼쪽
	}

	// 2. 다음 변 = 끝 꼭짓점에서 나가는 변 중 왼쪽 > 직진 > 오른쪽.
	//    꼭짓점에서 나가는 변은 1개(보통) 또는 2개(대각선 접촉). 2개일 때 왼쪽을 고르면 대각선 칸끼리 분리된다.
	//    이 규칙으로 "다음 변"은 변마다 하나로 정해지고 서로 겹치지 않으므로, 어느 변에서 출발해도 그 변으로 돌아온다
	TMultiMap<FIntPoint, int32> OutEdges;
	for (int32 Index = 0; Index < Edges.Num(); ++Index)
	{
		OutEdges.Add(Edges[Index].From, Index);
	}

	// 현재 변이 A -> B 라면, 다음 변은 반드시 B에서 시작하는 변 중 하나여야함.
	// 그 후보들을 TurnRank(D, C) 순으로 정렬해서 가장 작은 것을 선택한다.
	const auto NextEdge = [&Edges, &OutEdges](int32 Current)
		{
			TArray<int32> Candidates;
			OutEdges.MultiFind(Edges[Current].To, Candidates);

			const FIntPoint D = DirOf(Edges[Current]);
			int32 Best = INDEX_NONE;
			int32 BestRank = MAX_int32;
			for (const int32 Candidate : Candidates)
			{
				const int32 Rank = TurnRank(D, DirOf(Edges[Candidate]));
				if (Rank < BestRank)
				{
					BestRank = Rank;
					Best = Candidate;
				}
			}
			return Best;
		};

	TArray<bool> Used;
	Used.Init(false, Edges.Num());

	// 외곽선 추적
	TArray<TArray<FIntPoint>> Loops;
	for (int32 Start = 0; Start < Edges.Num(); ++Start)
	{
		// 이미 사용한 변이면 스킵
		if (Used[Start])
		{
			continue;
		}

		TArray<FIntPoint> Corners;
		int32 Current = Start;
		for (int32 Guard = 0; Guard <= Edges.Num(); ++Guard)   // 규칙이 깨져도 무한 반복하지 않게
		{
			Used[Current] = true;
			Corners.Add(Edges[Current].From);
			Current = NextEdge(Current);
			if (Current == Start || Current == INDEX_NONE)
			{
				break;
			}
		}
		ensureMsgf(Current == Start, TEXT("GridOutline: loop did not close"));

		// 3. 직선 위의 중간 꼭짓점 제거 (꺾이는 점만 남김)
		TArray<FIntPoint> Loop;
		const int32 N = Corners.Num();
		for (int32 Index = 0; Index < N; ++Index)
		{
			const FIntPoint In = Corners[Index] - Corners[(Index + N - 1) % N];
			const FIntPoint Out = Corners[(Index + 1) % N] - Corners[Index];
			if (In.X * Out.Y - In.Y * Out.X != 0)
			{
				Loop.Add(Corners[Index]);
			}
		}
		Loops.Add(MoveTemp(Loop));
	}
	return Loops;
}

double GridOutline::SignedArea(const TArray<FIntPoint>& Loop)
{
	const int32 N = Loop.Num();
	if (Loop.Num() < 3)
	{
		return 0.0;
	}

	double Area = 0;
	for (int32 Index = 0; Index < N; ++Index)
	{
		const FIntPoint& A = Loop[Index];
		const FIntPoint& B = Loop[(Index + 1) % N]; // 마지막 점에서 첫 점으로 이어지는 변도 포함
		Area += static_cast<double>(A.X) * B.Y - static_cast<double>(B.X) * A.Y;
	}
	return Area / 2.0; // 부호 유지함
}

SYNCHROPOST_API FVector GridOutline::CornerToWorld(const UGridManager& Grid, const FIntPoint& Corner)
{
	FVector Location = Grid.GetTileWorldLocation(Corner);
	Location -= FVector(Grid.GetTileSize() * 0.5f, Grid.GetTileSize() * 0.5f, 0.f);
	return Location;
}

bool GridOutline::IsDebugEnabled()
{
	return CVarDebugGridOutline.GetValueOnGameThread() > 0;
}

void GridOutline::DrawDebugLoops(const UWorld* World, const TArray<TArray<FIntPoint>>& Loops, float Duration)
{
#if ENABLE_DRAW_DEBUG
	const UGridManager* Grid = World ? World->GetSubsystem<UGridManager>() : nullptr;
	if (!Grid)
	{
		return;
	}
	const FVector Lift(0.f, 0.f, 8.f); // 바닥에 안묻히게 함
	for (const TArray<FIntPoint>& Loop : Loops)
	{
		const FColor Color = SignedArea(Loop) < 0.f ? FColor::Orange : FColor::Green;
		const int32 N = Loop.Num();
		for (int32 Index = 0; Index < N; ++Index)
		{
			const FVector A = CornerToWorld(*Grid, Loop[Index]) + Lift;
			const FVector B = CornerToWorld(*Grid, Loop[(Index + 1) % N]) + Lift;
			DrawDebugDirectionalArrow(World, A, B, 40.0f, Color, false, Duration, 0, 3.0f);
			DrawDebugPoint(World, A, 12.0f, Color, false, Duration);
		}
	}
#endif
}

// 까다로운 모양을 그리드 원점 근처에 그려서 눈으로 확인한다 (게임 상태와 무관, 벽 위에도 그림)
static void DrawGridOutlineTestShapes(UWorld* World)
{
	const TArray<FIntPoint> Cross = { {1, 0}, {0, 1}, {1, 1}, {2, 1}, {1, 2} };                       // 기대: 고리 1 (꼭짓점 12, 넓이 5)
	const TArray<FIntPoint> Ring = { {0, 0}, {1, 0}, {2, 0}, {0, 1}, {2, 1}, {0, 2}, {1, 2}, {2, 2} }; // 기대: 고리 2 (넓이 9, -1)
	const TArray<FIntPoint> Pinch = { {0, 0}, {1, 1} };                                               // 기대: 고리 2 (넓이 1, 1)
	const TArray<FIntPoint> LShape = { {0, 0}, {1, 0}, {0, 1}, {0, 2} };                              // 기대: 고리 1 (꼭짓점 6, 넓이 4)

	int32 OffsetX = 0;
	for (const TArray<FIntPoint>& Shape : { Cross, Ring, Pinch, LShape })
	{
		TArray<FIntPoint> Tiles = Shape;
		for (FIntPoint& Tile : Tiles)
		{
			Tile.X += OffsetX;
		}

		const TArray<TArray<FIntPoint>> Loops = GridOutline::BuildLoops(Tiles);
		GridOutline::DrawDebugLoops(World, Loops, 15.0f);

		FString Summary;
		for (const TArray<FIntPoint>& Loop : Loops)
		{
			Summary += FString::Printf(TEXT(" [corners %d, area %.0f]"), Loop.Num(), GridOutline::SignedArea(Loop));
		}
		UE_LOG(LogSP, Log, TEXT("[GridOutline] X=%d: %d loops%s"), OffsetX, Loops.Num(), *Summary);

		OffsetX += 4;
	}
}

static FAutoConsoleCommandWithWorld GGridOutlineTestShapes(
	TEXT("sp.GridOutline.TestShapes"),
	TEXT("십자 / 구멍 / 대각선 접촉 / L자 외곽선을 그리드 원점 근처에 15초간 그린다"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&DrawGridOutlineTestShapes));

#include "FrameWork/GridManager.h"
#include "Grid/TileMapDataAsset.h"
#include "Unit/GridMoveComponent.h"
#include "Unit/Unit.h"

void UGridManager::LoadGrid(UTileMapDataAsset* StageData)
{
    if (!StageData) return;

    TileGrid.InitializeGrid(StageData->GridWidth, StageData->GridHeight, StageData->TileSize);

    for (const FTileSpawnInfo& SpecialTile : StageData->SpecialTiles)
    {
        TileGrid.SetTileType(SpecialTile.Coordinate, SpecialTile.TileType);
    }
}

AUnit* UGridManager::GetUnitAt(const FIntPoint& Coord) const
{
    const FTile* Tile = TileGrid.Find(Coord);
    return Tile ? Tile->OccupyingUnit : nullptr;
}

bool UGridManager::IsWalkable(const FIntPoint& Coord) const
{
    const FTile* Tile = TileGrid.Find(Coord);
    return Tile && Tile->IsWalkable();
}

void UGridManager::SetUnitAt(const FIntPoint& Coord, AUnit* Unit)
{
    TileGrid.SetUnitAt(Coord, Unit);
    if (Unit)
    {
		Unit->SetGridPosition(Coord);
        Unit->OnUnitDied.AddDynamic(this, &UGridManager::HandleUnitDied);
    }
}

void UGridManager::ClearUnitAt(const FIntPoint& Coord)
{
    TileGrid.ClearUnitAt(Coord);
}

void UGridManager::SetTileType(const FIntPoint& Coord, ETileType NewType)
{
    TileGrid.SetTileType(Coord, NewType);
}

void UGridManager::MoveUnitAt(const FIntPoint& ToCoord, AUnit* Unit)
{
    if (!Unit)
    {
        return;
    }

    const FIntPoint FromCoord = Unit->GetGridPosition();

    // 방어: 실제로 그 자리를 이 유닛이 점유하고 있을 때만 비운다
    if (GetUnitAt(FromCoord) == Unit)
    {
        ClearUnitAt(FromCoord);
    }

    TileGrid.SetUnitAt(ToCoord, Unit);
    Unit->SetGridPosition(ToCoord);
}

TArray<FMoveStep> UGridManager::MoveUnitAlongPath(AUnit* Unit, const TArray<FIntPoint>& Path)
{
    TArray<FMoveStep> Steps;

    if(!Unit || Path.Num() == 0)
    {
        return Steps;
	}

    FIntPoint SegmentStart = Unit->GetGridPosition();
	FIntPoint LastCoord = SegmentStart;

    for (const FIntPoint& Coord : Path)
    {
        LastCoord = Coord;
		bool bAnyTriggered = false;
        if (FTileTriggerList* Found = TileTriggers.Find(Coord))
        {
            // 순회 중 트리거가 자기 자신을 제거할 수도 있어서 복사본으로 순회
            TArray<TScriptInterface<ITileTrigger>> TriggersHere = Found->Triggers;

            for (TScriptInterface<ITileTrigger>& Trigger : TriggersHere)
            {
                if (!Trigger.GetObject())
                { 
                    continue;
                }

                FCombatEventTarget Result = ITileTrigger::Execute_OnUnitEnter(Trigger.GetObject(), Unit);

                if (Result.Target.IsValid())
                {
                    if (!bAnyTriggered)
                    {
                        // 여기까지의 구간을 먼저 기록
                        FMoveStep SegmentStep;
                        SegmentStep.StepType = EMoveStepType::Segment;
                        SegmentStep.From = SegmentStart;
                        SegmentStep.To = Coord;
                        Steps.Add(SegmentStep);

                        SegmentStart = Coord;
                        bAnyTriggered = true;
                    }

                    FMoveStep TriggerStep;
                    TriggerStep.StepType = EMoveStepType::Trigger;
                    TriggerStep.From = Coord;
                    TriggerStep.To = Coord;
                    TriggerStep.Trigger = Trigger;
                    TriggerStep.Result = Result;
                    Steps.Add(TriggerStep);
                }
            }
        }
        if (bAnyTriggered)
        {
            UGridMoveComponent* MoveComp = Unit->GetGridMoveComponent(); // 6번 태스크에서 추가될 함수
            if (MoveComp && !MoveComp->CanMove())
            {
                MoveUnitAt(Coord, Unit); // 여기서 조기 중단
                return Steps;
            }
        }
    }
    if (SegmentStart != LastCoord)
    {
        FMoveStep FinalSegment;
        FinalSegment.StepType = EMoveStepType::Segment;
        FinalSegment.From = SegmentStart;
        FinalSegment.To = LastCoord;
        Steps.Add(FinalSegment);
    }

    MoveUnitAt(LastCoord, Unit);
    return Steps;
}

void UGridManager::AddTileTrigger(const FIntPoint& Coord, TScriptInterface<ITileTrigger> Trigger)
{
    TileTriggers.FindOrAdd(Coord).Triggers.Add(Trigger);
}

void UGridManager::RemoveTileTrigger(const FIntPoint& Coord, TScriptInterface<ITileTrigger> Trigger)
{
    if (FTileTriggerList* Found = TileTriggers.Find(Coord))
    {
        Found->Triggers.Remove(Trigger);
        if (Found->Triggers.Num() == 0)
        {
            TileTriggers.Remove(Coord);
        }
    }
}

void UGridManager::HandleUnitDied(AUnit* Unit)
{
    if (!Unit)
    {
        return;
    }

	ClearUnitAt(Unit->GetGridPosition());
}

FGridReachability UGridManager::GetReachableTiles(const FIntPoint& Start, int32 MaxRange) const
{
    FGridReachability Result;
    Result.DistanceFromStart.Add(Start, 0);

    TQueue<FIntPoint> Frontier;
    Frontier.Enqueue(Start);

    static const TArray<FIntPoint> Directions = {
        FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1)
    };

    FIntPoint Current;
    while (Frontier.Dequeue(Current))
    {
        const int32 CurrentDist = Result.DistanceFromStart[Current];
        if (CurrentDist >= MaxRange)
        {
            continue; // 더 이상 못 뻗어나감
        }

        for (const FIntPoint& Dir : Directions)
        {
            const FIntPoint Next = Current + Dir;

            if (Result.DistanceFromStart.Contains(Next))
            {
                continue; // BFS라서 이미 방문했으면 더 짧은 경로는 없음
            }

            if (!IsWalkable(Next) || GetUnitAt(Next) != nullptr)
            {
                continue; // 못 지나감 (벽이거나 다른 유닛이 점유중)
            }

            Result.DistanceFromStart.Add(Next, CurrentDist + 1);
            Result.CameFrom.Add(Next, Current);
            Frontier.Enqueue(Next);
        }
    }

    return Result;
}

TArray<FIntPoint> UGridManager::FindPath(const FIntPoint& Start, const FIntPoint& Destination, int32 MaxRange) const
{
    TArray<FIntPoint> Path;

    if (!IsWalkable(Destination) || GetUnitAt(Destination) != nullptr)
    {
        return Path; // 목적지 자체가 막혀있음
    }

    TMap<FIntPoint, int32> GScore;
    TMap<FIntPoint, FIntPoint> CameFrom;
    TSet<FIntPoint> OpenSet;
    TSet<FIntPoint> ClosedSet;

    GScore.Add(Start, 0);
    OpenSet.Add(Start);

	// 방향 벡터: 상하좌우
    static const TArray<FIntPoint> Directions = {
        FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1)
    };

    // 목적지 방향 휴리스틱 + 직선 이탈 페널티(대각선처럼 보이게 하는 핵심)
    auto Heuristic = [&](const FIntPoint& Coord) -> float
        {
            const int32 Manhattan = FMath::Abs(Coord.X - Destination.X) + FMath::Abs(Coord.Y - Destination.Y);

            const int32 Dx1 = Coord.X - Destination.X;
            const int32 Dy1 = Coord.Y - Destination.Y;
            const int32 Dx2 = Start.X - Destination.X;
            const int32 Dy2 = Start.Y - Destination.Y;
            const float Cross = FMath::Abs(Dx1 * Dy2 - Dx2 * Dy1);

            return Manhattan + Cross * 0.001f; // 0.001은 "동률일 때만 영향" 주기 위한 아주 작은 가중치
        };

    while (OpenSet.Num() > 0)
    {
        // OpenSet에서 FScore가 가장 낮은 노드를 선택
        FIntPoint Current;
        float BestFScore = TNumericLimits<float>::Max();
        for (const FIntPoint& Candidate : OpenSet)
        {
            const float F = GScore[Candidate] + Heuristic(Candidate);
            if (F < BestFScore)
            {
                BestFScore = F;
                Current = Candidate;
            }
        }

        // 목적지에 도착했으면 경로를 복원한다.
        if (Current == Destination)
        {
            FIntPoint Node = Current;
            Path.Add(Node);
			// 경로 복원: CameFrom를 따라가면서 경로를 역순으로 구성
            while (const FIntPoint* Parent = CameFrom.Find(Node))
            {
                Node = *Parent;
                Path.Add(Node);
            }
			// 경로를 뒤집어서 시작점에서 목적지로 가는 순서로 만든다.
            Algo::Reverse(Path);
            Path.RemoveAt(0); // Start 제외

			// 경로 길이가 MaxRange를 초과하면 빈 배열 반환
            return (Path.Num() <= MaxRange) ? Path : TArray<FIntPoint>();
        }

		// 방문할 후보에서 제거하고, 이미 방문한 것으로 표시
        OpenSet.Remove(Current);
        ClosedSet.Add(Current);

		// 현재 노드의 GScore가 MaxRange 이상이면 더 이상 진행하지 않는다.
        const int32 CurrentG = GScore[Current];
        if (CurrentG >= MaxRange)
        {
            continue;
        }

		// 현재 노드의 이웃 노드를 탐색
        for (const FIntPoint& Dir : Directions)
        {
            const FIntPoint Next = Current + Dir;

            // 이미 탐색했거나, 못가는 타일이면 건너뜀.
            if (ClosedSet.Contains(Next) || !IsWalkable(Next))
            {
                continue;
            }

            const int32 TentativeG = CurrentG + 1;

			// 만약 이웃 노드가 GScore에 없거나(안 가본 곳), 더 짧은 경로를 찾았으면 업데이트
            if (!GScore.Contains(Next) || TentativeG < GScore[Next])
            {
                GScore.Add(Next, TentativeG);
                CameFrom.Add(Next, Current);
                OpenSet.Add(Next);
            }
        }
    }

    return Path; // 못 찾음 (빈 배열)
}

FVector UGridManager::GetTileWorldLocation(const FIntPoint& Coord) const
{
    const FTile* Tile = TileGrid.Find(Coord);
    return Tile ? Tile->WorldLocation : FVector::ZeroVector;
}

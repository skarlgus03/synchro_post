
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

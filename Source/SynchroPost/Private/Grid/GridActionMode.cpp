
#include "Grid/GridActionMode.h"
#include "Unit/Unit.h"
#include "Framework/GridManager.h"
#include "Unit/GridMoveComponent.h"
#include "Unit/SkillComponent.h"

TArray<FIntPoint> UMoveActionMode::GetRangeTiles() const
{
	if (!ActingUnit || !ActingUnit->GetGridMoveComponent())
	{
		return {};
	}

	UGridManager* GridManager = ActingUnit->GetWorld()->GetSubsystem<UGridManager>();
	if (!GridManager)
	{
		return {};
	}

	const FGridReachability Reachability = GridManager->GetReachableTiles(
		ActingUnit->GetGridPosition(), 
		ActingUnit->GetGridMoveComponent()->GetAvailableMovePoint());

	TArray<FIntPoint> RangeTiles;
	Reachability.DistanceFromStart.GetKeys(RangeTiles);
	RangeTiles.Remove(ActingUnit->GetGridPosition()); // 현재 위치는 제외
	return RangeTiles;
}

bool UMoveActionMode::IsValidTarget(const FIntPoint& HoveredCoord) const
{
	return GetRangeTiles().Contains(HoveredCoord);
}

TArray<FIntPoint> UMoveActionMode::ComputeRelatedTiles(const FIntPoint& HoveredCoord) const
{
	if(!ActingUnit || !ActingUnit->GetGridMoveComponent())
	{
		return {};
	}

	UGridManager* GridManager = ActingUnit->GetWorld()->GetSubsystem<UGridManager>();
	if (!GridManager)
	{
		return {};
	}

	return GridManager->FindPath(
		ActingUnit->GetGridPosition(), HoveredCoord, 
		ActingUnit->GetGridMoveComponent()->GetAvailableMovePoint()
	);
}

void UMoveActionMode::ConfirmAction(const FIntPoint& HoveredCoord) const
{
	if (!ActingUnit) return;
	ActingUnit->ServerRequestMove(HoveredCoord);
}

TArray<FIntPoint> USkillActionMode::GetRangeTiles() const
{
	if (!ActingUnit || !ActingUnit->GetSkillComponent())
	{
		return {};
	}

	return ActingUnit->GetSkillComponent()->GetSkillRangeTiles(SkillSlotTag);
}

bool USkillActionMode::IsValidTarget(const FIntPoint& HoveredCoord) const
{
	if (!ActingUnit || !ActingUnit->GetSkillComponent())
	{
		return false;
	}

	return ActingUnit->GetSkillComponent()->GetValidTargetTiles(SkillSlotTag).Contains(HoveredCoord);
}

TArray<FIntPoint> USkillActionMode::ComputeRelatedTiles(const FIntPoint& HoveredCoord) const
{
	if (!ActingUnit || !ActingUnit->GetSkillComponent())
	{
		return {};
	}

	// 범위 공격 로직 필요
	return ActingUnit->GetSkillComponent()->GetAffectedTiles(SkillSlotTag, HoveredCoord);
}

void USkillActionMode::ConfirmAction(const FIntPoint& HoveredCoord) const
{
	if (!ActingUnit) return;

	FSkillTargetData TargetData;
	TargetData.SelectedTiles.Add(HoveredCoord); // RangePatternOffsets에 의한 확산은 서버의 ApplyEffectToTargets가 처리

	ActingUnit->ServerExecuteSkill(SkillSlotTag, TargetData);
}





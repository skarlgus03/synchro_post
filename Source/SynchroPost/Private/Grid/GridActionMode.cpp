
#include "Grid/GridActionMode.h"
#include "Unit/Unit.h"
#include "Framework/GridManager.h"
#include "Unit/GridMoveComponent.h"
#include "Unit/SkillComponent.h"
#include "SynchroPost.h"
#include "Framework/SPPlayerController.h"


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
	RangeTiles.Remove(ActingUnit->GetGridPosition());
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
	if (!ActingUnit || !OwningController)
	{
		UE_LOG(LogSP, Error, TEXT("[Move] ConfirmAction 실패 - Unit 또는 Controller 없음"));
		return;
	}

	OwningController->Server_RequestMove(ActingUnit, HoveredCoord);
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
	if (!ActingUnit || !OwningController)
	{
		UE_LOG(LogSP, Error, TEXT("[Skill] ConfirmAction 실패 - Unit 또는 Controller 없음"));
		return;
	}

	FSkillTargetData TargetData;
	TargetData.SelectedTiles = this->SelectedTiles;

	OwningController->Server_ExecuteSkill(ActingUnit, SkillSlotTag, TargetData);

}

bool USkillActionMode::RequiresMultipleSelections() const
{
	if (!ActingUnit || !ActingUnit->GetSkillComponent())
	{
		return false;
	}
	return ActingUnit->GetSkillComponent()->GetSkillData(SkillSlotTag).TargetingRule.RequiredTileSelectionCount > 1;
}

bool USkillActionMode::RegisterSelection(const FIntPoint& SelectedCoord)
{
	SelectedTiles.AddUnique(SelectedCoord);

	if (!ActingUnit || !ActingUnit->GetSkillComponent())
	{
		return false;
	}

	const int32 RequiredCount = ActingUnit->GetSkillComponent()->GetSkillData(SkillSlotTag).TargetingRule.RequiredTileSelectionCount;
	return SelectedTiles.Num() >= RequiredCount;
}



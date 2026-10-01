#include "AI/DummyAIBrain.h"
#include "Unit/Unit.h"
#include "Unit/SkillComponent.h"
#include "Unit/GridMoveComponent.h"
#include "Framework/GridManager.h"
#include "Types/SPSkillStructure.h"
#include "EngineUtils.h"
#include "SynchroPost.h"

void UDummyAIBrain::ExecuteTurn_Implementation()
{
	AUnit* Self = GetOwnerUnit();
	if (!Self)
	{
		FinishTurn();
		return;
	}

	int32 ActionCount = 0;
	for (; ActionCount < MaxActionsPerTurn; ++ActionCount)
	{
		if (Self->IsDead())
		{
			break; // 반격·함정 등으로 행동 중에 죽을 수 있다
		}
		if (TryUseBestSkill(Self))
		{
			continue;
		}
		if (TryMoveTowardNearestHostile(Self))
		{
			continue;
		}
		break; // 때릴 것도, 더 다가갈 곳도 없음
	}

	if (ActionCount >= MaxActionsPerTurn)
	{
		UE_LOG(LogSP, Warning, TEXT("[AI] %s: 최대 행동 수(%d) 도달 — 비용 0 스킬이 있는지 확인"),
			*Self->GetName(), MaxActionsPerTurn);
	}

	FinishTurn();
}

bool UDummyAIBrain::TryUseBestSkill(AUnit* Self)
{
	USkillComponent* SkillComp = Self->GetSkillComponent();
	if (!SkillComp)
	{
		return false;
	}

	FGameplayTag BestSkill;
	FIntPoint BestTile = FIntPoint::ZeroValue;
	int32 BestScore = 0;

	// 스킬 목록 순서대로 탐색해서 가장 높은 점수의 스킬을 사용함.
	for (const FSkillEntry& Entry : SkillComp->GetSkillEntries())
	{
		const FGameplayTag SkillTag = Entry.SkillSlotTag;
		if (!Entry.Skill || !SkillComp->CanExecuteSkill(SkillTag))
		{
			continue; // 쿨다운·자원·침묵
		}

		const FSkillData Data = SkillComp->GetSkillData(SkillTag);
		const FSkillTargetingRule& Rule = Data.TargetingRule;

		if (Rule.RequiredTileSelectionCount != 1)
		{
			continue; // 다중 선택 스킬(연사 등)은 더미에서 제외
		}
		if (Rule.TargetFaction != ESkillTargetFaction::Enemy && Rule.TargetFaction != ESkillTargetFaction::Any)
		{
			continue; // 적에게 효과가 가지 않는 스킬
		}

		for (const FIntPoint& Tile : SkillComp->GetValidTargetTiles(SkillTag))
		{
			const int32 Score = CountHostilesInTiles(Self, SkillComp->GetAffectedTiles(SkillTag, Tile));
			if (Score > BestScore) // 동률이면 먼저 찾은 것(스킬 목록 순서) 유지
			{
				BestScore = Score;
				BestSkill = SkillTag;
				BestTile = Tile;
			}
		}
	}

	if (BestScore <= 0)
	{
		return false;
	}

	FSkillTargetData Target;
	Target.SelectedTiles.Add(BestTile);

	const bool bSuccess = SkillComp->ExecuteSkill(BestSkill, Target);
	UE_LOG(LogSP, Log, TEXT("[AI] %s: 스킬 %s @ (%d,%d) / 적 %d명 → %s"),
		*Self->GetName(), *BestSkill.ToString(), BestTile.X, BestTile.Y, BestScore,
		bSuccess ? TEXT("성공") : TEXT("실패"));
	return bSuccess;
}

bool UDummyAIBrain::TryMoveTowardNearestHostile(AUnit* Self)
{
	UGridMoveComponent* MoveComp = Self->GetGridMoveComponent();
	UGridManager* Grid = GetWorld() ? GetWorld()->GetSubsystem<UGridManager>() : nullptr;
	if (!MoveComp || !Grid)
	{
		return false;
	}

	const int32 MovePoint = MoveComp->GetAvailableMovePoint();
	if (MovePoint <= 0)
	{
		return false;
	}

	const AUnit* Target = FindNearestHostile(Self);
	if (!Target)
	{
		return false;
	}

	const FIntPoint From = Self->GetGridPosition();
	const FIntPoint Goal = Target->GetGridPosition();

	FIntPoint BestTile = From;
	int32 BestDist = GridDistance(From, Goal);
	int32 BestCost = 0;

	const FGridReachability Reach = Grid->GetReachableTiles(From, MovePoint);
	for (const TPair<FIntPoint, int32>& Pair : Reach.DistanceFromStart)
	{
		const int32 Dist = GridDistance(Pair.Key, Goal);
		// 더 가까운 칸 우선, 같으면 덜 걷는 칸
		if (Dist < BestDist || (Dist == BestDist && BestTile != From && Pair.Value < BestCost))
		{
			BestTile = Pair.Key;
			BestDist = Dist;
			BestCost = Pair.Value;
		}
	}

	if (BestTile == From)
	{
		return false; // 이미 가장 가까움
	}

	const bool bSuccess = MoveComp->RequestMove(BestTile);
	UE_LOG(LogSP, Log, TEXT("[AI] %s: 목표=%s(거리 %d) → 이동 (%d,%d)→(%d,%d) %s"),
		*Self->GetName(), *Target->GetName(), GridDistance(From, Goal),
		From.X, From.Y, BestTile.X, BestTile.Y, bSuccess ? TEXT("성공") : TEXT("실패"));
	return bSuccess;
}

AUnit* UDummyAIBrain::FindNearestHostile(const AUnit* Self) const
{
	AUnit* Nearest = nullptr;
	int32 NearestDist = MAX_int32;

	for (TActorIterator<AUnit> It(GetWorld()); It; ++It)
	{
		AUnit* Other = *It;
		if (!Other || Other->IsDead() || !IsHostile(Self, Other))
		{
			continue;
		}
		const int32 Dist = GridDistance(Self->GetGridPosition(), Other->GetGridPosition());
		if (Dist < NearestDist)
		{
			NearestDist = Dist;
			Nearest = Other;
		}
	}
	return Nearest;
}

int32 UDummyAIBrain::CountHostilesInTiles(const AUnit* Self, const TArray<FIntPoint>& Tiles) const
{
	// Tiles에 있는 적 유닛 수를 센다. (죽은 유닛 제외)
	const UGridManager* Grid = GetWorld() ? GetWorld()->GetSubsystem<UGridManager>() : nullptr;
	if (!Grid)
	{
		return 0;
	}

	int32 Count = 0;
	for (const FIntPoint& Tile : Tiles)
	{
		const AUnit* Unit = Grid->GetUnitAt(Tile);
		if (Unit && !Unit->IsDead() && IsHostile(Self, Unit))
		{
			++Count;
		}
	}
	return Count;
}

bool UDummyAIBrain::IsHostile(const AUnit* Self, const AUnit* Other)
{
	if (!Self || !Other || Self == Other)
	{
		return false;
	}
	const EFaction A = Self->GetFaction();
	const EFaction B = Other->GetFaction();
	return A != B && A != EFaction::Neutral && B != EFaction::Neutral;
}
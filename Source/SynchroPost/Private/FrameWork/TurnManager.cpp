#include "Framework/TurnManager.h"
#include "Unit/Unit.h"
#include "Unit/SkillComponent.h"

void UTurnManager::StartCombat(const TArray<AUnit*>& InParticipants)
{
	Participants.Reset();
	for (AUnit* Unit : InParticipants)
	{
		if (Unit)
		{
			Participants.Add(Unit);
			Unit->OnUnitRevived.AddUniqueDynamic(this, &UTurnManager::HandleUnitRevived);
			Unit->OnUnitDied.AddUniqueDynamic(this, &UTurnManager::HandleUnitDied);
			UE_LOG(LogTemp, Log, TEXT("StartCombat: 참가자 등록 - %s"), *Unit->GetName());
		}
	}
	UE_LOG(LogTemp, Log, TEXT("StartCombat: 총 %d명 등록됨"), Participants.Num());

	bCombatActive = true;
	CurrentRound = 0;
	BeginRound();
}

void UTurnManager::EndCurrentUnitTurn()
{
	if (!CurrentUnit)
	{
		return;
	}

	AUnit* FinishedUnit = CurrentUnit;

	ActedThisRound.Add(FinishedUnit);
	OnUnitTurnEnd.Broadcast(FinishedUnit);

	AdvanceToNextUnit();
}

void UTurnManager::StartUnitTurn(AUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	CurrentUnit = Unit;
	OnUnitTurnStart.Broadcast(CurrentUnit);
}

void UTurnManager::CheckCombatEndCondition()
{
	if (!bCombatActive)
	{
		UE_LOG(LogTemp, Log, TEXT("CheckCombatEndCondition: bCombatActive=false라서 스킵"));
		return;
	}
	bool bAnyPlayerAlive = false;
	bool bAnyEnemyAlive = false;

	for (AUnit* Unit : Participants)
	{
		if (!Unit || Unit->IsDead())
		{
			continue;
		}

		if (Unit->GetFaction() == EFaction::Player)
		{
			bAnyPlayerAlive = true;
		}
		else if (Unit->GetFaction() == EFaction::Enemy)
		{
			bAnyEnemyAlive = true;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CheckCombatEndCondition: Participants=%d, bAnyPlayerAlive=%s, bAnyEnemyAlive=%s"),
		Participants.Num(), bAnyPlayerAlive ? TEXT("true") : TEXT("false"), bAnyEnemyAlive ? TEXT("true") : TEXT("false"));


	if (bAnyPlayerAlive && !bAnyEnemyAlive)
	{
		bCombatActive = false;
		OnCombatEnd.Broadcast(ECombatResult::Victory);
	}
	else if (bAnyEnemyAlive && !bAnyPlayerAlive)
	{
		bCombatActive = false;
		OnCombatEnd.Broadcast(ECombatResult::Defeat);
	}
}

void UTurnManager::HandleUnitRevived(AUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	if (ActedThisRound.Contains(Unit))
	{
		// 이미 행동한 유닛이면, 이번 라운드에는 행동하지 않음
		return;
	}

	if (PendingQueue.Contains(Unit))
	{
		// 이미 대기열에 있는 유닛이면, 중복 추가하지 않음
		return;
	}

	PendingQueue.Add(Unit);
	PendingQueue.Sort([](const AUnit& A, const AUnit& B) {
		return A.GetSpeed() > B.GetSpeed();
		});
}

void UTurnManager::HandleUnitDied(AUnit* Unit)
{
	UE_LOG(LogTemp, Log, TEXT("HandleUnitDied: Unit=%s"), Unit ? *Unit->GetName() : TEXT("NULL"));
	CheckCombatEndCondition();
}

void UTurnManager::BeginRound()
{
	CurrentRound++;

	PendingQueue.Reset();

	for (AUnit* Unit : Participants)
	{
		if (IsValidParticipant(Unit))
		{
			PendingQueue.Add(Unit);
		}
	}

	// 참가자가 없으면 전투 종료
	if (PendingQueue.Num() == 0)
	{
		bCombatActive = false;
		OnCombatEnd.Broadcast(ECombatResult::Defeat);
		return;
	}


	PendingQueue.Sort([](const AUnit& A, const AUnit& B) {
		return A.GetSpeed() > B.GetSpeed();
		});
	
	ActedThisRound.Reset();

	OnRoundStart.Broadcast(CurrentRound);

	AdvanceToNextUnit();
}

void UTurnManager::EndRound()
{
	OnRoundEnd.Broadcast(CurrentRound);

	if (bCombatActive)
	{
		BeginRound();
	}
}

void UTurnManager::AdvanceToNextUnit()
{
	if (!bCombatActive)
	{
		return;
	}
	
	// 남은 유닛이 있지만, 유효하지 않은 유닛이면 제거함.
	while (PendingQueue.Num() > 0 && !IsValidParticipant(PendingQueue[0]))
	{
		PendingQueue.RemoveAt(0);
	}

	// 남은 유닛이 없으면 라운드를 종료함.
	if (PendingQueue.Num() == 0)
	{
		EndRound();
		return;
	}

	AUnit* NextUnit = PendingQueue[0];
	PendingQueue.RemoveAt(0);

	StartUnitTurn(NextUnit);
}

bool UTurnManager::IsValidParticipant(AUnit* Unit) const
{
	return Unit != nullptr && !Unit->IsDead();
}





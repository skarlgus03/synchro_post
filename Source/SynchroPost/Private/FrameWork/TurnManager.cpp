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
		}
	}

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

	BeginRound();
}

void UTurnManager::AdvanceToNextUnit()
{
	
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





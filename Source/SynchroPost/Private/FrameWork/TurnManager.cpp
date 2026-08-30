#include "Framework/TurnManager.h"
#include "Unit/Unit.h"
#include "Unit/SkillComponent.h"
#include "Framework/TurnStateComponent.h"
#include "Framework/SPGameState.h"

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
	if (UTurnStateComponent* TurnState = GetTurnStateComponent())
	{
		TurnState->SetCurrentRound(0);
	}

	BeginRound();
}

void UTurnManager::EndCurrentUnitTurn()
{
	UTurnStateComponent* TurnState = GetTurnStateComponent();
	if (!TurnState || !TurnState->GetCurrentUnit())
	{
		return;
	}
	
	ActedThisRound.Add(TurnState->GetCurrentUnit());
	AdvanceToNextUnit();
}

void UTurnManager::StartUnitTurn(AUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("StartUnitTurn: %s의 턴 시작"), *Unit->GetName());
	
	if (UTurnStateComponent* TurnState = GetTurnStateComponent())
	{
		TurnState->SetCurrentUnit(Unit);
	}
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
	if (!Unit || ActedThisRound.Contains(Unit))
	{
		return;
	}
	
	UTurnStateComponent* TurnState = GetTurnStateComponent();
	if (!TurnState)
	{
		return;
	}

	TArray<TObjectPtr<AUnit>> PendingQueue = TurnState->GetPendingQueue();
	if (PendingQueue.Contains(Unit))
	{
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
	UTurnStateComponent* TurnState = GetTurnStateComponent();
	if (!TurnState) return;

	TurnState->SetCurrentRound(TurnState->GetCurrentRound() + 1);

	ActedThisRound.Reset();

	TArray<TObjectPtr<AUnit>> NewQueue;

	UE_LOG(LogTemp, Warning, TEXT("BeginRound: Participants=%d"), Participants.Num());
	for (AUnit* Unit : Participants)
	{
		if (IsValidParticipant(Unit))
		{
			NewQueue.Add(Unit);
		}
	}

	// 참가자가 없으면 전투 종료
	if (NewQueue.Num() == 0)
	{
		bCombatActive = false;
		OnCombatEnd.Broadcast(ECombatResult::Defeat);
		return;
	}


	NewQueue.Sort([](const AUnit& A, const AUnit& B) {
		return A.GetSpeed() > B.GetSpeed();
		});
	
	ActedThisRound.Reset();
	TurnState->SetPendingQueue(NewQueue);
	OnRoundStart.Broadcast(GetCurrentRound());

	AdvanceToNextUnit();
}

void UTurnManager::EndRound()
{
	if (UTurnStateComponent* TurnState = GetTurnStateComponent())
	{
		OnRoundEnd.Broadcast(TurnState->GetCurrentRound());
	}
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

	UTurnStateComponent* TurnState = GetTurnStateComponent();
	if (!TurnState)
	{
		return;
	}
	
	TArray<TObjectPtr<AUnit>> PendingQueue = TurnState->GetPendingQueue();

	// 남은 유닛이 있지만, 유효하지 않은 유닛이면 제거함.
	while (PendingQueue.Num() > 0 && !IsValidParticipant(PendingQueue[0]))
	{
		PendingQueue.RemoveAt(0);
	}

	// 남은 유닛이 없으면 라운드를 종료함.
	if (PendingQueue.Num() == 0)
	{
		TurnState->SetPendingQueue(PendingQueue);
		EndRound();
		return;
	}

	AUnit* NextUnit = PendingQueue[0];
	PendingQueue.RemoveAt(0);
	TurnState->SetPendingQueue(PendingQueue);

	StartUnitTurn(NextUnit);
}

bool UTurnManager::IsValidParticipant(AUnit* Unit) const
{
	return Unit != nullptr && !Unit->IsDead();
}

UTurnStateComponent* UTurnManager::GetTurnStateComponent() const
{
	if (!GetWorld())
	{
		return nullptr;
	}
	ASPGameState* GS = GetWorld()->GetGameState<ASPGameState>();
	return GS ? GS->GetTurnStateComponent() : nullptr;
}

AUnit* UTurnManager::GetCurrentUnit() const
{
	UTurnStateComponent* TurnState = GetTurnStateComponent();
	return TurnState ? TurnState->GetCurrentUnit() : nullptr;
}

int32 UTurnManager::GetCurrentRound() const
{
	UTurnStateComponent* TurnState = GetTurnStateComponent();
	return TurnState ? TurnState->GetCurrentRound() : 0;
}

const TArray<AUnit*>& UTurnManager::GetPendingQueue() const
{
	static const TArray<TObjectPtr<AUnit>> EmptyQueue;
	UTurnStateComponent* TurnState = GetTurnStateComponent();
	return TurnState ? TurnState->GetPendingQueue() : EmptyQueue;
}
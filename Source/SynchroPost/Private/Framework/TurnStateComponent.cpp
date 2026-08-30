#include "Framework/TurnStateComponent.h"
#include "Net/UnrealNetwork.h"

UTurnStateComponent::UTurnStateComponent()
{

	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTurnStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTurnStateComponent, CurrentUnit);
	DOREPLIFETIME(UTurnStateComponent, CurrentRound);
	DOREPLIFETIME(UTurnStateComponent, PendingQueue);
}

void UTurnStateComponent::SetCurrentUnit(AUnit* NewCurrentUnit)
{
	AUnit* OldUnit = CurrentUnit;
	CurrentUnit = NewCurrentUnit;

	if (OldUnit)
	{
		OnUnitTurnEnd.Broadcast(OldUnit);
	}
	if (CurrentUnit)
	{
		OnUnitTurnStart.Broadcast(CurrentUnit);
	}
}

void UTurnStateComponent::SetCurrentRound(int32 NewRound)
{
	CurrentRound = NewRound;
	OnRoundChanged.Broadcast(CurrentRound);
}

void UTurnStateComponent::SetPendingQueue(const TArray<TObjectPtr<AUnit>> NewQueue)
{
	PendingQueue = NewQueue;
	OnPendingQueueChanged.Broadcast();
}

void UTurnStateComponent::OnRep_CurrentUnit(AUnit* OldCurrentUnit)
{
	if (OldCurrentUnit)
	{
		OnUnitTurnEnd.Broadcast(OldCurrentUnit);
	}
	if (CurrentUnit)
	{
		OnUnitTurnStart.Broadcast(CurrentUnit);
	}
}

void UTurnStateComponent::OnRep_CurrentRound()
{
	OnRoundChanged.Broadcast(CurrentRound);
}

void UTurnStateComponent::OnRep_PendingQueue()
{
	OnPendingQueueChanged.Broadcast();
}







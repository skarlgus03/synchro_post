#include "Unit/StateComponent.h"
#include "Net/UnrealNetwork.h"
#include "Unit/Unit.h"
#include "Framework/TurnManager.h"
#include "StatusEffect/StatusEffectBase.h"
#include "SynchroPost.h"
#include "StatusEffect/StatusEffectDataAsset.h"

// Sets default values for this component's properties
UStateComponent::UStateComponent()
{
	
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UStateComponent::BeginPlay()
{
	Super::BeginPlay();

	
	OwnerUnit = Cast<AUnit>(GetOwner());
}


void UStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UStateComponent, StateTagList);
}

FGameplayTagContainer UStateComponent::GetStateTags() const
{
	FGameplayTagContainer Container;
	for (const FStateTagEntry& Entry : StateTagList.Entries)
	{
		Container.AddTag(Entry.GetTag());
	}
	return Container;
}

bool UStateComponent::HasStateTag(const FGameplayTag& Tag) const
{
	return StateTagList.FindFirst(Tag) != nullptr;
}

int32 UStateComponent::GetStatusEffectCount(const FGameplayTag& Tag) const
{
	int32 Count = 0;
	for (const FStateTagEntry& Entry : StateTagList.Entries)
	{
		if (Entry.GetTag() == Tag)
		{
			Count += Entry.StackCount; // StackCounter는 StackCount가 곧 중첩 수, Independent는 엔트리마다 StackCount=1이라 결국 개수 카운트
		}
	}
	return Count;
}

bool UStateComponent::RemoveFirstEffectByTag(const FGameplayTag& Tag)
{
	if (FStateTagEntry* Entry = StateTagList.FindFirst(Tag))
	{
		RemoveStatusEffectInstance(Entry->EffectInstance);
		OnStateTagRefreshed.Broadcast();
		return true;
	}
	return false;
}

void UStateComponent::ApplyStatusEffect(const UStatusEffectDataAsset* Effect, int32 Duration, AActor* Source)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (!Effect)
	{
		UE_LOG(LogSP, Warning, TEXT("[State] ApplyStatusEffect 실패 - Effect가 nullptr"));
	}



}


void UStateComponent::RemoveStatusEffectInstance(UStatusEffectBase* Instance)
{
	if (GetOwnerRole() != ROLE_Authority || !Instance)
	{
		return;
	}

	Instance->OnRemove();
	RemoveReplicatedSubObject(Instance);

	StateTagList.RemoveByInstance(Instance);
}

void UStateComponent::ReduceDurationByOneTurn()
{
	// 서버에서만 호출 가능
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	TArray<FStateTagEntry> Expired = StateTagList.ReduceDurationsAndGetExpired();

	for (const FStateTagEntry& ExpiredEntry : Expired)
	{
		RemoveStatusEffectInstance(ExpiredEntry.EffectInstance);
	}

}

void UStateComponent::OnRep_StateTags()
{
	OnStateTagRefreshed.Broadcast();
}

void UStateComponent::HandleUnitTurnStart(AUnit* Unit)
{
	if (Unit != OwnerUnit)
	{
		return;
	}

	for (FStateTagEntry& Entry : StateTagList.Entries)
	{
		Entry.EffectInstance->OnTurnStart();
	}
}

void UStateComponent::HandleUnitTurnEnd(AUnit* Unit)
{
	if (Unit != OwnerUnit)
	{
		return;
	}

	for (FStateTagEntry& Entry : StateTagList.Entries)
	{
		Entry.EffectInstance->OnTurnEnd();
	}

	ReduceDurationByOneTurn();

	OnStateTagRefreshed.Broadcast();
}

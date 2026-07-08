#include "Unit/StateComponent.h"
#include "Net/UnrealNetwork.h"

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
		Container.AddTag(Entry.StateTag);
	}
	return Container;
}

bool UStateComponent::HasStateTag(const FGameplayTag& Tag) const
{
	return StateTagList.Find(Tag) != nullptr;
}

void UStateComponent::AddStateTag(const FGameplayTag& Tag, int32 Duration)
{ 
	// 서버만 가능
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	bool bWasNewlyAdded = StateTagList.AddOrUpdate(Tag, Duration);

	if (bWasNewlyAdded)
	{
		OnStateTagRefreshed.Broadcast();
	}
}

void UStateComponent::RemoveStateTag(const FGameplayTag& Tag)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (StateTagList.Remove(Tag))
	{
		OnStateTagRefreshed.Broadcast();
	}
}

void UStateComponent::ReduceDurationByOneTurn()
{
	// 서버에서만 호출 가능
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	TArray<FGameplayTag> ExpiredTags = StateTagList.ReduceDurations();

	for (const FGameplayTag& Tag : ExpiredTags)
	{
		OnStateTagRefreshed.Broadcast();
	}

}

void UStateComponent::OnRep_StateTags()
{
	OnStateTagRefreshed.Broadcast();
}

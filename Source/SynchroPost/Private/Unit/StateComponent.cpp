#include "Unit/StateComponent.h"
#include "Net/UnrealNetwork.h"
#include "StatusEffect/StatusEffectBase.h"

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

	if (UWorld* World = GetWorld())
	{
		CachedTurnManager = World->GetSubsystem<UTurnManager>();
		// 델리게이트 바인딩
		if (CachedTurnManager)
		{
			CachedTurnManager->OnTurnStart.AddDynamic(this, &UStateComponent::HandleUnitTurnStart);
			CachedTurnManager->OnTurnEnd.AddDynamic(this, &UStateComponent::HandleUnitTurnEnd);
		}
	}
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

void UStateComponent::AddStateTag(const FGameplayTag& Tag, int32 Duration, TSubclassOf<UStatusEffectBase> EffectClass)
{ 
	// 서버만 가능
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	
	UStatusEffectBase* EffectInstance = nullptr;
	if (EffectClass)
	{
		EffectInstance = NewObject<UStatusEffectBase>(this, EffectClass);
		EffectInstance->SetOwnerComponent(this);
		EffectInstance->EffectTag = Tag;
		AddReplicatedSubObject(EffectInstance);
	}

	bool bWasNewlyAdded = StateTagList.AddOrUpdate(Tag, Duration, EffectInstance);

	if (bWasNewlyAdded && EffectInstance)
	{
		EffectInstance->OnApply();
		OnStateTagRefreshed.Broadcast();
	}
}

void UStateComponent::RemoveStateTag(const FGameplayTag& Tag)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (FStateTagEntry* Entry = StateTagList.Find(Tag))
	{
		if (Entry->EffectInstance)
		{
			Entry->EffectInstance->OnRemove();
			RemoveReplicatedSubObject(Entry->EffectInstance);
		}
	}

	if (StateTagList.Remove(Tag))
	{
		OnStateTagsRefreshed.Broadcast();
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

void UStateComponent::HandleUnitTurnStart(AUnit* Unit)
{
	if (Unit != GetOwner())
	{
		return;
	}

	for (FStateTagEntry& Entry : StateTagList.Entries)
	{
		if (Entry.EffectInstance)
		{
			Entry.EffectInstance->OnTurnStart();
		}
	}
}

void UStateComponent::HandleUnitTurnEnd(AUnit* Unit)
{
	if (Unit != GetOwner())
	{
		return;
	}
	for (FStateTagEntry& Entry : StateTagList.Entries)
	{
		if (Entry.EffectInstance)
		{
			Entry.EffectInstance->OnTurnEnd();
		}
	}


	TArray<FGameplayTag> ExpiredTags = StateTagList.ReduceDurations();

	for (const FGameplayTag& Tag : ExpiredTags)
	{
		RemoveStateTag(Tag);
	}

	OnStateTagRefreshed.Broadcast();
}

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
	for (const FStateTagEntry& Entry : StateTagList.Entries)
	{
		if (Entry.GetTag().MatchesTag(Tag))
		{
			return true;
		}
	}
	return false;
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
		RemoveStatusEffect(*Entry);
		OnStateTagRefreshed.Broadcast();
		return true;
	}
	return false;
}

bool UStateComponent::ApplyStatusEffect(const UStatusEffectDataAsset* Effect, int32 Duration, AActor* Source)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}

	if (!Effect)
	{
		UE_LOG(LogSP, Error, TEXT("[State] ApplyStatusEffect - Effect가 null"));
		return false;
	}
	if (!Effect->Logic)
	{
		UE_LOG(LogSP, Error, TEXT("[State] %s의 Logic이 비어 있음"), *GetNameSafe(Effect));
		return false;
	}

	if (Duration == 0 || Duration < StatusEffectConst::Infinite)
	{
		UE_LOG(LogSP, Error, TEXT("[State] %s의 Duration이 0 또는 음수임"), *GetNameSafe(Effect));
		return false;
	}


	FStateTagEntry* Existing = (Effect->StackingPolicy != EStackingPolicy::Independent)
		? StateTagList.FindFirst(Effect->StatusEffectTag) : nullptr;

	if (Existing)
	{
		if (Effect->StackingPolicy == EStackingPolicy::RefreshDuration)
		{
			StateTagList.RefreshDuration(*Existing, Duration);
		}
		else if (Effect->StackingPolicy == EStackingPolicy::StackCounter)
		{
			StateTagList.IncrementStack(*Existing, Duration, Effect->MaxStackCount);
		}
		OnStateTagRefreshed.Broadcast();
		return true;
	}

	UStatusEffectBase* NewInstance = DuplicateObject<UStatusEffectBase>(Effect->Logic, this);
	NewInstance->SetOwnerComponent(this);
	NewInstance->Source = Source;
	FStateTagEntry NewEntry(Effect, Duration, NewInstance);
	StateTagList.Add(NewEntry);
	NewInstance->OnApply(NewEntry);
	OnStateTagRefreshed.Broadcast();
	return true;
	
}

void UStateComponent::RemoveStatusEffect(FStateTagEntry Entry)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (!Entry.EffectInstance)
	{
		ensure(false);
		return;
	}

	Entry.EffectInstance->OnRemove(Entry);
	StateTagList.RemoveByInstance(Entry.EffectInstance);
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
		RemoveStatusEffect(ExpiredEntry);
	}

}

void UStateComponent::OnRep_StateTags()
{
	OnStateTagRefreshed.Broadcast();
}

void UStateComponent::HandleUnitTurnStart(AUnit* Unit)
{
	if (GetOwnerRole() != ROLE_Authority || Unit != OwnerUnit)
	{
		return;
	}

	// 복사본 만들어서 하기. (제거될 수 있으므로)
	const TArray<FStateTagEntry> SnapShot = StateTagList.Entries;
	for (const FStateTagEntry& Entry : SnapShot)
	{
		if (Entry.EffectInstance)
		{
			Entry.EffectInstance->OnTurnStart(Entry);
		}
	}
}

void UStateComponent::HandleUnitTurnEnd(AUnit* Unit)
{
	if (GetOwnerRole() != ROLE_Authority || Unit != OwnerUnit)
	{
		return;
	}

	// 복사본 만들어서 하기. (제거될 수 있으므로)
	const TArray<FStateTagEntry> SnapShot = StateTagList.Entries;
	for (const FStateTagEntry& Entry : SnapShot)
	{
		if (Entry.EffectInstance)
		{
			Entry.EffectInstance->OnTurnEnd(Entry);
		}
	}

	ReduceDurationByOneTurn();
	OnStateTagRefreshed.Broadcast();
}


#include "Unit/SkillComponent.h"
#include "Skill/SkillBase.h"
#include "Unit/UnitDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Skill/SkillDataAsset.h"
#include "Unit/StateComponent.h"
#include "Unit/Unit.h"

// Sets default values for this component's properties
USkillComponent::USkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

}

// Called when the game starts
void USkillComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerUnit = Cast<AUnit>(GetOwner());
	CachedStateComponent = GetOwner()->FindComponentByClass<UStateComponent>();
}


void USkillComponent::InitializeSkillComponent(UUnitDataAsset* UnitDataAsset)
{
	if (!UnitDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("USkillComponent::InitializeSkillComponent - UnitDataAsset is null."));
		return;
	}

	for (const auto& Pair : UnitDataAsset->SkillDataAssetMap)
	{
		const FGameplayTag& SkillSlotTag = Pair.Key;
		USkillDataAsset* SkillDataAsset = Pair.Value;

		if (!SkillDataAsset || !SkillDataAsset->SkillLogicClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("USkillComponent::InitializeSkillComponent - SkillDataAsset or SkillLogicClass is null for tag: %s"), *SkillSlotTag.ToString());
			continue;
		}

		// Create the skill instance
		USkillBase* SkillInstance = NewObject<USkillBase>(this, SkillDataAsset->SkillLogicClass);
		if (SkillInstance)
		{
			SkillInstance->InitializeSkill(SkillDataAsset);
		}

		// Add the skill entry to the skill list
		FSkillEntry NewSkillEntry;
		NewSkillEntry.SkillSlotTag = SkillSlotTag;
		NewSkillEntry.Skill = SkillInstance;
		SkillList.AddSkill(SkillSlotTag, SkillInstance);

		this->AddReplicatedSubObject(SkillInstance);

		
	}
}

USkillBase* USkillComponent::FindSkillByTag(const FGameplayTag& SkillSlotTag) const
{
	
	for (const FSkillEntry& Entry : SkillList.Entries)
	{
		if (Entry.SkillSlotTag == SkillSlotTag)
		{
			return Entry.Skill;
		}
	}
	return nullptr;
}

bool USkillComponent::CanExecuteSkill(const FGameplayTag& SkillSlotTag) const
{
	// if Character is dead or stunned, return false
	if (!CheckCommonState())
	{
		return false;
	}

	// Find SkillBase by SkillSlotTag
	USkillBase* Skill = FindSkillByTag(SkillSlotTag);
	if (!Skill)
	{
		return false;
	}

	FSkillExecutionContext Context = BuildExecutionContext();

	const FSkillData& SkillData = Skill->GetCurrentSkillData(Context.StateTags);

	// Checking Unit's resources against the skill's cost
	for (const FSkillResource& Cost : SkillData.SkillCost)
	{
		if (!HasEnoughResource(Cost.ResourceTag, Cost.Value))
		{
			return false;
		}
	}


	// Checking Skill's cooldown
	if (Skill->GetCurrentCooldown(Context.StateTags) > 0)
	{
		return false;
	}
	
	// Checking Skill's own conditions (like cooldown, etc.)
	if (!Skill->CheckSkillCondition(Context))
	{
		return false;
	}

	return true;
}

void USkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USkillComponent, SkillList);
	DOREPLIFETIME(USkillComponent, CurrentResources);
}

void USkillComponent::OnRep_CurrentResources()
{
}

bool USkillComponent::HasEnoughResource(const FGameplayTag& ResourceTag, int32 RequireValue) const
{
	if (const FSkillResource* Found = FindResource(ResourceTag))
	{
		return Found->Value >= RequireValue;
	}
	return false;
}

bool USkillComponent::CheckCommonState() const
{
	return true;
}

FGameplayTagContainer USkillComponent::GetSiblingStateTag() const
{
	if (CachedStateComponent)
	{
		return CachedStateComponent->GetStateTags();
	}
	return FGameplayTagContainer::EmptyContainer;
}

FSkillExecutionContext USkillComponent::BuildExecutionContext() const
{
	FSkillExecutionContext Context;

	Context.StateTags = GetSiblingStateTag();
	if (OwnerUnit)
	{
		Context.CasterCoordinate = OwnerUnit->GetGridPosition();
		Context.CasterFaction = OwnerUnit->GetFaction();
	}

	return Context;
}
	



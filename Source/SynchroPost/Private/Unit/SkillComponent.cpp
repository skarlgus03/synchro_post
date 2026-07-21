
#include "Unit/SkillComponent.h"
#include "Skill/SkillBase.h"
#include "Unit/UnitDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Skill/SkillDataAsset.h"
#include "Unit/StateComponent.h"
#include "Framework/TurnManager.h"
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

	if (UWorld* World = GetWorld())
	{
		if (CachedTurnManager = World->GetSubsystem<UTurnManager>())
		{
			CachedTurnManager->OnUnitTurnEnd.AddDynamic(this, &USkillComponent::HandleUnitTurnEnd);
		}
	}
}


void USkillComponent::InitializeSkillComponent(UUnitDataAsset* UnitDataAsset)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

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
			SkillInstance->SetOwnerComponent(this);

			// Add the skill entry to the skill list
			SkillList.AddSkill(SkillSlotTag, SkillInstance);
			this->AddReplicatedSubObject(SkillInstance);
		}
				
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
	return CanExecuteSkill(SkillSlotTag, BuildExecutionContext());
}

bool USkillComponent::CanExecuteSkill(const FGameplayTag& SkillSlotTag, const FSkillExecutionContext& Context) const
{
	if (!CheckCommonState())
	{
		return false;
	}

	USkillBase* Skill = FindSkillByTag(SkillSlotTag);
	if (!Skill)
	{
		return false;
	}


	const FSkillData& SkillData = Skill->GetCurrentSkillData(Context.StateTags);

	// Blocking Tag 검사
	if (Context.StateTags.HasAny(SkillData.BlockingTags))
	{
		return false;
	}

	// 쿨다운 검사
	if (Skill->GetCurrentCooldown(Context.StateTags) > 0)
	{
		return false;
	}

	// 스킬 리소스 검사
	for (const FSkillResource& Cost : SkillData.SkillCost)
	{
		if (!HasEnoughResource(Cost.ResourceTag, Cost.Value))
		{
			return false;
		}
	}

	return Skill->CheckSkillCondition(Context);
}

bool USkillComponent::ExecuteSkill(const FGameplayTag& SkillSlotTag, const FSkillTargetData& Target)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecuteSkill should be called on the server."));
		return false;
	}

	USkillBase* Skill = FindSkillByTag(SkillSlotTag);
	if (!Skill)
	{
		UE_LOG(LogTemp, Warning, TEXT("Skill not found for tag: %s"), *SkillSlotTag.ToString());
		return false;
	}

	FSkillExecutionContext Context = BuildExecutionContext();

	if (!CanExecuteSkill(SkillSlotTag, Context))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot execute skill for tag: %s"), *SkillSlotTag.ToString());
		return false;
	}
	if(!Skill->CanExecuteOnTarget(Target, Context))
	{
		UE_LOG(LogTemp, Warning, TEXT("Skill cannot be executed on the provided target for tag: %s"), *SkillSlotTag.ToString());
		return false;
	}

	const FSkillData& SkillData = Skill->GetCurrentSkillData(Context.StateTags);
	for (const FSkillResource& Cost : SkillData.SkillCost)
	{
		ConsumeResource(Cost.ResourceTag, Cost.Value);
	}
	Skill->ExecuteSkill(Target, Context);

	return true;
}

void USkillComponent::HandleUnitTurnStart(AUnit* Unit)
{
	if (Unit == OwnerUnit)
	{
		// Handle turn start logic here if needed
	}
}

void USkillComponent::HandleUnitTurnEnd(AUnit* Unit)
{
	if (Unit == OwnerUnit)
	{
		ReduceCooldownsByOneTurn();
	}
}

void USkillComponent::ReduceCooldownsByOneTurn()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	for (FSkillEntry& Entry : SkillList.Entries)
	{
		if (Entry.Skill)
		{
			Entry.Skill->DecreaseCooldowns();
		}
	}
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

void USkillComponent::ConsumeResource(const FGameplayTag& ResourceTag, int32 Amount)
{
	if (FSkillResource* Found = FindResource(ResourceTag))
	{
		Found->Value = FMath::Max(0, Found->Value - Amount);
	}
}
	



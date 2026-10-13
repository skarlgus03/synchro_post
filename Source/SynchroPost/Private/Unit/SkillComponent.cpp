
#include "Unit/SkillComponent.h"
#include "Skill/SkillBase.h"
#include "Unit/UnitDataAsset.h"
#include "Unit/UnitStatDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Skill/SkillDataAsset.h"
#include "Unit/StateComponent.h"
#include "Framework/TurnManager.h"
#include "Framework/CombatEventComponent.h"
#include "Unit/Unit.h"
#include "SynchroPost.h"

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


void USkillComponent::InitializeSkillComponent(const UUnitDataAsset* UnitDataAsset)
{
	if (!UnitDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("USkillComponent::InitializeSkillComponent - UnitDataAsset is null."));
		return;
	}

	SkillList.Entries.Empty();
	CurrentResources.Empty();
	SkillInstances.Empty();

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
		USkillBase* SkillInstance = GetOrCreateInstance(SkillDataAsset);
		if (SkillInstance)
		{
			SkillList.AddSkill(SkillSlotTag, SkillInstance);
		}
				
	}


	if (UnitDataAsset->UnitStatData)
	{
		CachedUnitStatData = UnitDataAsset->UnitStatData;
		for (const FSkillResource& Resource : UnitDataAsset->UnitStatData->SkillResources)
		{
			CurrentResources.Add(Resource);
		}
	}

}

USkillBase* USkillComponent::FindSkillByTag(const FGameplayTag& SkillSlotTag) const
{
	
	for (const FSkillEntry& Entry : SkillList.Entries)
	{
		if (Entry.SkillSlotTag == SkillSlotTag)
		{
			// 태그는 맞는데 객체가 비어 있는 경우를 조용히 넘기지 않는다.
			// (이 침묵이 클라 연출 누락 버그의 진단을 며칠 늦췄다)
			if (!Entry.Skill)
			{
				UE_LOG(LogTemp, Error, TEXT("[Skill] 엔트리는 있으나 Skill이 null | Owner=%s Auth=%d Tag=%s"),
					*GetNameSafe(GetOwner()),
					GetOwnerRole() == ROLE_Authority ? 1 : 0,
					*SkillSlotTag.ToString());
			}
			return Entry.Skill;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Skill] FindSkillByTag 실패 - 엔트리 없음 | Owner=%s Auth=%d Tag=%s (보유 %d개)"),
		*GetNameSafe(GetOwner()),
		GetOwnerRole() == ROLE_Authority ? 1 : 0,
		*SkillSlotTag.ToString(),
		SkillList.Entries.Num());
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


	const FSkillData& SkillData = Skill->GetSkillData();

	// Blocking Tag 검사
	if (Context.StateTags.HasAny(SkillData.BlockingTags))
	{
		return false;
	}

	// 쿨다운 검사
	if (Skill->GetCurrentCooldown() > 0)
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
	
	// 스킬을 하나의 액션으로 묶기
	FCombatActionScope ActionScope(OwnerUnit ? OwnerUnit->GetCombatEventComponent() : nullptr);
	
	USkillBase* Skill = FindSkillByTag(SkillSlotTag);
	if (!Skill)
	{
		UE_LOG(LogTemp, Warning, TEXT("Skill not found for tag: %s"), *SkillSlotTag.ToString());
		return false;
	}

	// 스킬 실행 컨텍스트를 구성
	FSkillExecutionContext Context = BuildExecutionContext();
	Context.SkillSlotTag = SkillSlotTag;
	Context.AimCoordinate = Target.SelectedTiles.IsEmpty() ? Context.CasterCoordinate : Target.SelectedTiles[0];

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

	const FSkillData& SkillData = Skill->GetSkillData();
	for (const FSkillResource& Cost : SkillData.SkillCost)
	{
		ConsumeResource(Cost.ResourceTag, Cost.Value);
	}
	Skill->ExecuteSkill(Target, Context);

	return true;
}

TArray<FIntPoint> USkillComponent::GetSkillRangeTiles(const FGameplayTag& SkillSlotTag) const
{
	USkillBase* Skill = FindSkillByTag(SkillSlotTag);
	return Skill ? Skill->GetRangeTiles(BuildExecutionContext()) : TArray<FIntPoint>();
}

TArray<FIntPoint> USkillComponent::GetValidTargetTiles(const FGameplayTag& SkillSlotTag) const
{
	USkillBase* Skill = FindSkillByTag(SkillSlotTag);
	return Skill ? Skill->GetValidTargetTiles(BuildExecutionContext()) : TArray<FIntPoint>();
}

TArray<FIntPoint> USkillComponent::GetAffectedTiles(const FGameplayTag& SkillSlotTag, const FIntPoint& TargetCoord) const
{
	USkillBase* Skill = FindSkillByTag(SkillSlotTag);
	return Skill ? Skill->GetAffectedTiles(TargetCoord, BuildExecutionContext()) : TArray<FIntPoint>();
}

USkillBase* USkillComponent::GetOrCreateInstance(USkillDataAsset* SkillAsset)
{
	if (!SkillAsset || !SkillAsset->SkillLogicClass)
	{
		UE_LOG(LogSP, Warning, TEXT("[Skill] 인스턴스 생성 실패: %s (DA 또는 LogicClass 없음)"), *GetNameSafe(SkillAsset));
		return nullptr;
	}

	if (TObjectPtr<USkillBase>* Found = SkillInstances.Find(SkillAsset))
	{
		return *Found;
	}

	USkillBase* Instance = NewObject<USkillBase>(this, SkillAsset->SkillLogicClass);
	Instance->InitializeSkill(SkillAsset);
	Instance->SetOwnerComponent(this);
	SkillInstances.Add(SkillAsset, Instance);
	return Instance;
}

void USkillComponent::HandleUnitTurnStart(AUnit* Unit)
{
	if (Unit == OwnerUnit)
	{
		RefillResource();
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
	for (const TPair<TObjectPtr<USkillDataAsset>, TObjectPtr<USkillBase>>& Pair : SkillInstances)
	{
		if (Pair.Value)
		{
			Pair.Value->DecreaseCooldown();
		}
	}
}

void USkillComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USkillComponent, CurrentResources);
}

FSkillData USkillComponent::GetSkillData(const FGameplayTag& SkillSlotTag) const
{
	USkillBase* Skill = FindSkillByTag(SkillSlotTag);
	return Skill->GetSkillData();
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

void USkillComponent::RefillResource()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	const UStatComponent* StatComp = OwnerUnit ? OwnerUnit->GetStatComponent() : nullptr;
	if (!StatComp)
	{
		return;
	}

	RebuildMaxResources();
	CurrentResources = MaxResources;
}

void USkillComponent::RebuildMaxResources()
{
	MaxResources.Reset();
	if (CachedUnitStatData)
	{
		MaxResources = CachedUnitStatData->SkillResources;
	}
}




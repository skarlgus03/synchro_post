#include "Unit/StatComponent.h"
#include "Unit/UnitDataAsset.h"
#include "FrameWork/SynchroPostSettings.h"
#include "UObject/UObjectGlobals.h"
#include "Unit/UnitResistanceDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Unit/Unit.h"
#include "Slot/UnitSlot.h"
#include "Math/DamageMath.h"



static constexpr int32 GimmickStatDefault = 1;
static constexpr int32 GimmickStatMin = 1;
static constexpr int32 GimmickStatMax = 10;

// Sets default values for this component's properties
UStatComponent::UStatComponent()
{
	// 틱 필요없어
	PrimaryComponentTick.bCanEverTick = false;

	// 네트워크 복제 필요함
	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UStatComponent::BeginPlay()
{
	Super::BeginPlay();
		
	OwnerUnit = Cast<AUnit>(GetOwner());
}

// Called every frame
void UStatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UStatComponent, CurrentHealth);
}

void UStatComponent::InitializeStats(const UUnitStatDataAsset* StatData, int32 Level)
{
	InitializeStatsToGlobalBaseValue();
	InitializeStatsFromUnitStatData(StatData);
	InitializeStatsFromLevel(Level);
	InitializeResistancesFromUnitStatData(StatData);
	RefreshAllStats();
}


void UStatComponent::InitializeStatsFromUnitStatData(const UUnitStatDataAsset* StatData)
{
	if (!StatData)
	{
		return;
	}

	CurrentStatData = StatData;

	for (const auto& Pair : StatData->BaseStats)
	{
		StatMap.FindOrAdd(Pair.Key).UnitBaseValue = Pair.Value;
	}
}

void UStatComponent::InitializeStatsFromLevel(int32 Level)
{
	if (!CurrentStatData)
	{
		UE_LOG(LogTemp, Warning, TEXT("CurrentStatData is null."));
		return;
	}

	if (Level <= 1) return;

	for (const auto& Pair : CurrentStatData->StatIncrements)
	{
		if (FUnitStat* TargetStat = StatMap.Find(Pair.Key))
		{
			TargetStat->UnitBaseValue += Pair.Value * (Level - 1);
		}
	}
}

void UStatComponent::InitializeResistancesFromUnitStatData(const UUnitStatDataAsset* StatData)
{
	Resistances.Empty();

	if (!StatData)
	{
		UE_LOG(LogTemp, Warning, TEXT("StatData is null."));
		return;
	}
	if (!StatData->ResistanceData)
	{
		UE_LOG(LogTemp, Warning, TEXT("ResistanceData is null in the provided StatData."));
		return;
	}
	for (const auto& Pair : StatData->ResistanceData->ResistanceMap)
	{
		Resistances.Add(Pair.Key, Pair.Value);
	}
}

void UStatComponent::InitializeStatsToGlobalBaseValue()
{
	StatMap.Empty();

	if (const USynchroPostSettings* Settings = GetDefault<USynchroPostSettings>())
	{
		// SoftObjectPtr이므로 안전하게 로드해서 사용
		if (UUnitStatDataAsset* SharedData = Settings->BaseStatDefaultData.LoadSynchronous())
		{
			for (const auto& Pair : SharedData->BaseStats)
			{
				FUnitStat NewStat;
				NewStat.UnitBaseValue = Pair.Value;
				StatMap.Add(Pair.Key, NewStat);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BaseStatDataAsset is null. please check Project Settings"));
		}
	}
}

void UStatComponent::InitializeGimmickStats(const UUnitDataAsset* UnitData)
{
	GimmickStatMap.Reset();
	if (!UnitData) return;

	for (const auto& Pair : UnitData->GimmickStats)
	{
		GimmickStatMap.Add(Pair.Key, FMath::Clamp(Pair.Value, GimmickStatMin, GimmickStatMax));
	}
}


float UStatComponent::GetDefenseMultiplier(const FGameplayTagContainer& Tags, const FPenetrationData& Penetration) const
{
	int32 Defense = 0;
	int32 Flat = 0;
	int32 Percent = 0;

	if (Tags.HasTag(SPTags::Action::Damage::Physical))
	{
		Defense = GetStat(SPTags::Stat::Combat::Primary::DefPhysical);
		Flat = Penetration.PhysicalFlat;
		Percent = Penetration.PhysicalPercent;
	}
	else if (Tags.HasTag(SPTags::Action::Damage::Magic))
	{
		Defense = GetStat(SPTags::Stat::Combat::Primary::DefMagic);
		Flat = Penetration.MagicalFlat;
		Percent = Penetration.MagicalPercent;
	}
	else
	{
		return 1.f; // 고정 피해 등은 방어 무시
	}

	return StatMath::DefenseToMultiplier(StatMath::EffectiveDefense(Defense, Flat, Percent));
}

float UStatComponent::GetResistanceMultiplier(const FGameplayTagContainer& Tags) const
{
	float Multiplier = 1.f;
	for (const FGameplayTag& Tag : Tags)
	{
		const int32 Resistance = GetResistance(Tag);
		if (Resistance != 0)
		{
			// 저항 100% 초과가 피해를 회복으로 뒤집지 않게 0에서 막는다. 음수(약점)는 1 초과 허용
			Multiplier *= StatMath::ResistanceToMultiplier(Resistance);
		}
	}
	return Multiplier;
}

void UStatComponent::UpdateCachedStatModifier()
{
	for (auto& Pair : StatMap)
	{
		Pair.Value.CachedModifier = FCachedStatModifier();
	}

	TArray<FStatModifierEntry> AllModifiers = SlotStatModifiers; 
	AllModifiers.Append(StatusEffectModifiers); 

	for (const FStatModifierEntry& ModifierEntry : AllModifiers)
	{
		if (FUnitStat* TargetStat = StatMap.Find(ModifierEntry.StatModifier.StatTag))
		{
			TargetStat->CachedModifier.FlatValue += ModifierEntry.StatModifier.FlatValue;
			TargetStat->CachedModifier.PercentValue += ModifierEntry.StatModifier.PercentValue;
		}
	}
}

void UStatComponent::OnRep_CurrentHealth()
{
	UE_LOG(LogTemp, Warning, TEXT("[NET] OnRep_Health: %d"), CurrentHealth);
	OnHealthChanged.Broadcast(CurrentHealth, FSPHealthActionData());
}

void UStatComponent::RefreshAllStats()
{
	const int32 OldMaxHealth = GetStat(SPTags::Stat::Combat::Primary::MaxHealth);

	UpdateCachedStatModifier();

	for (auto& Pair : StatMap)
	{
		Pair.Value.UpdateFinalValue();
	}

	const int32 NewMaxHealth = GetStat(SPTags::Stat::Combat::Primary::MaxHealth);

	const int32 HealthDelta = NewMaxHealth - OldMaxHealth;

	CurrentHealth = FMath::Clamp(CurrentHealth + HealthDelta, 0, NewMaxHealth);
}


FSPHealthActionData UStatComponent::ApplyHealthChange(const FSPHealthActionData& ActionData)
{
	if (!GetOwner()->HasAuthority())
	{
		return FSPHealthActionData();
	}

	const bool bIsHeal = ActionData.ActionTypeTags.HasTag(SPTags::Action::Heal);
	return bIsHeal ? ApplyHeal(ActionData) : ApplyDamage(ActionData);
}

FSPHealthActionData UStatComponent::ApplyDamage(FSPHealthActionData ActionData)
{
	const int32 Total = DamageMath::ResolveDamage(ActionData,
		[this](const FGameplayTagContainer& PartTags, const FPenetrationData& Pen) { return GetDefenseMultiplier(PartTags, Pen); },
		[this](const FGameplayTagContainer& PartTags) { return GetResistanceMultiplier(PartTags); });

	// 만약에 추가 고정감소 있다면 여기서 할 수 있음.

	CurrentHealth = FMath::Clamp(CurrentHealth - Total, 0, GetStat(SPTags::Stat::Combat::Primary::MaxHealth));

	if (OnHealthChanged.IsBound())
	{
		OnHealthChanged.Broadcast(CurrentHealth, ActionData);   // 타격당 한 번
	}
	return ActionData;
}

FSPHealthActionData UStatComponent::ApplyHeal(FSPHealthActionData HealData)
{
	HealData.EnsureParts();
	HealData.SyncTotalsFromParts();

	CurrentHealth = FMath::Clamp(CurrentHealth + HealData.Amount, 0, GetStat(SPTags::Stat::Combat::Primary::MaxHealth));

	if (OnHealthChanged.IsBound())
	{
		OnHealthChanged.Broadcast(CurrentHealth, HealData);
	}

	return HealData;
}

void UStatComponent::SetSlotModifiers(const TArray<FStatModifierEntry>& NewModifiers)
{
	SlotStatModifiers = NewModifiers;
	RefreshAllStats();
}

void UStatComponent::AddStatusEffectModifiers(const TArray<FStatModifierEntry>& NewModifiers)
{
	StatusEffectModifiers.Append(NewModifiers);
	RefreshAllStats();
}

void UStatComponent::RemoveStatusEffectModifiers(UObject* Source)
{
	StatusEffectModifiers.RemoveAll([Source](const FStatModifierEntry& Entry)
	{
		return Entry.Source == Source;
	});
	RefreshAllStats();
}

void UStatComponent::ModifyGimmickStat(FGameplayTag StatTag, int32 Delta)
{
	int32 Current = GetGimmickStat(StatTag);
	GimmickStatMap.Add(StatTag, FMath::Clamp(Current + Delta, GimmickStatMin, GimmickStatMax));
}

int32 UStatComponent::GetStat(FGameplayTag StatTag) const
{
	if (const FUnitStat* Stat = StatMap.Find(StatTag))
	{
		return Stat->FinalValue;
	}
	return 0;
}

int32 UStatComponent::GetResistance(FGameplayTag StatTag) const
{
	if (const int32* Resistance = Resistances.Find(StatTag))
	{
		return *Resistance;
	}
	return 0;
}

int32 UStatComponent::GetGimmickStat(FGameplayTag StatTag) const
{
	if (const int32* GimmickStat = GimmickStatMap.Find(StatTag))
	{
		return *GimmickStat;
	}
	return GimmickStatDefault;
}

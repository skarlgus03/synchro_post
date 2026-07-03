#include "Unit/UnitStatComponent.h"
#include "Unit/UnitDataAsset.h"
#include "FrameWork/SynchroPostSettings.h"
#include "UObject/UObjectGlobals.h"
#include "Unit/UnitResistanceDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Unit/Unit.h"
#include "Slot/UnitSlot.h"

// Sets default values for this component's properties
UUnitStatComponent::UUnitStatComponent()
{
	// 틱 필요없어
	PrimaryComponentTick.bCanEverTick = false;

	// 네트워크 복제 필요함
	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UUnitStatComponent::BeginPlay()
{
	Super::BeginPlay();

	
	InitializeStatsToGlobalBaseValue();

	OwnerUnit = Cast<AUnit>(GetOwner());
}

// Called every frame
void UUnitStatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UUnitStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UUnitStatComponent, CurrentHealth);
}

void UUnitStatComponent::InitializeStats(const UUnitStatDataAsset* StatData, int32 Level)
{
	InitializeStatsToGlobalBaseValue();
	InitializeStatsFromUnitStatData(StatData);
	InitializeStatsFromLevel(Level);
	InitializeResistancesFromUnitStatData(StatData);
	RefreshAllStats();
}


void UUnitStatComponent::InitializeStatsFromUnitStatData(const UUnitStatDataAsset* StatData)
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

void UUnitStatComponent::InitializeStatsFromLevel(int32 Level)
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

void UUnitStatComponent::InitializeResistancesFromUnitStatData(const UUnitStatDataAsset* StatData)
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

void UUnitStatComponent::InitializeStatsToGlobalBaseValue()
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


void UUnitStatComponent::CalculateDamageAfterDefense(FSPDamageData& DamageData)
{
	int32 Defense = 0;
	int32 Flat = 0;
	int32 Percent = 0;

	// 1. 물리 / 마법 대미지 유형에 따른 스탯 가로채기 
	if (DamageData.DamageTypeTags.HasTag(SPTags::Damage::Form::Physical))
	{
		Defense = GetStat(SPTags::Stat::Combat::Primary::DefPhysical);
		Flat = DamageData.PenetrationData.PhysicalFlat;
		Percent = DamageData.PenetrationData.PhysicalPercent;
	}
	else if (DamageData.DamageTypeTags.HasTag(SPTags::Damage::Form::Magic))
	{
		Defense = GetStat(SPTags::Stat::Combat::Primary::DefMagic);
		Flat = DamageData.PenetrationData.MagicalFlat;
		Percent = DamageData.PenetrationData.MagicalPercent;
	}
	else
	{
		// 물리나 마법 대미지가 아니라면(고정 피해 등) 방어력 정산을 건너뛴다.
		return;
	}

	// 2. 관통력 계산 
	const float PercentPenRate = StatMath::PercentToFloat(Percent);
	const float DefenseAfterPercent = (float)Defense * (1.0f - PercentPenRate);

	// 최종 유효 방어력 산출
	const int32 FinalDefense = FMath::Max(FMath::RoundToInt32(DefenseAfterPercent) - Flat, 0);

	// 3. 방어력 효율 곡선 공식 대입
	const float DefenseMultiplier = 100.f / (100.f + (float)FinalDefense);
	const float FinalCalculatedDamage = (float)DamageData.RawDamage * DefenseMultiplier;

	// 4. 택배 상자 내부의 대미지를 최종 정산 값으로 직접 갱신
	DamageData.RawDamage = FMath::Max(1, FMath::RoundToInt32(FinalCalculatedDamage));
}

void UUnitStatComponent::CalculateDamageAfterResistance(FSPDamageData& DamageData)
{

	float CurrentCalculatedDamage = (float)DamageData.RawDamage;

	for (const FGameplayTag& Tag : DamageData.DamageTypeTags)
	{
		int32 ResistanceValue = GetResistance(Tag);
		if (ResistanceValue != 0)
		{
			const float ResistanceRate = StatMath::PercentToFloat(ResistanceValue);

			CurrentCalculatedDamage = CurrentCalculatedDamage * (1.0f - ResistanceRate);
		}
	}

	DamageData.RawDamage = FMath::Max(0, FMath::RoundToInt(CurrentCalculatedDamage));
}

void UUnitStatComponent::UpdateCachedStatModifier()
{
	for (auto& Pair : StatMap)
	{
		Pair.Value.CachedModifier = FCachedStatModifier();
	}

	const TMap<FGameplayTag, FCachedStatModifier>& CachedModifiers = OwnerUnit->GetCurrentSlot()->GetCachedStatModifiers();
	for (const auto& Pair : CachedModifiers)
	{
		if (FUnitStat* Stat = StatMap.Find(Pair.Key))
		{
			Stat->CachedModifier = Pair.Value;
		}
	}
}

void UUnitStatComponent::RefreshAllStats()
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

int32 UUnitStatComponent::ApplyDamage(const FSPDamageData& DamageData)
{
	if (!GetOwner()->HasAuthority())
	{
		return 0;
	}

	FSPDamageData CalcData = DamageData;

	CalculateDamageAfterDefense(CalcData);

	CalculateDamageAfterResistance(CalcData);


	CurrentHealth = FMath::Clamp(CurrentHealth - CalcData.RawDamage, 0, GetStat(SPTags::Stat::Combat::Primary::MaxHealth));
	if (OnHealthChanged.IsBound())
	{
		OnHealthChanged.Broadcast(CurrentHealth, CalcData.RawDamage, DamageData.DamageTypeTags);
	}

	return CalcData.RawDamage;
}

int32 UUnitStatComponent::GetStat(FGameplayTag StatTag)
{
	if (const FUnitStat* Stat = StatMap.Find(StatTag))
	{
		return Stat->FinalValue;
	}
	return 0;
}

int32 UUnitStatComponent::GetResistance(FGameplayTag StatTag)
{
	if (const int32* Resistance = Resistances.Find(StatTag))
	{
		return *Resistance;
	}
	return 0;
}
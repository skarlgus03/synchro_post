// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/UnitStatComponent.h"
#include "Data/UnitDataAsset.h"
#include "FrameWork/SynchroPostSettings.h"
#include "UObject/UObjectGlobals.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UUnitStatComponent::UUnitStatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.

	// 틱 필요없어
	PrimaryComponentTick.bCanEverTick = false;

	// 네트워크 복제 필요함
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UUnitStatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...sl 
	
	InitializeStatsToGlobalBaseValue();
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
	InitializeStatsFromUnitStatData(StatData);
	InitializeStatsFromLevel(Level);
	RefreshAllStats();
}

void UUnitStatComponent::InitializeStatsFromUnitStatData(const UUnitStatDataAsset* StatData)
{
	if (!StatData) return;
	CurrentStatData = StatData;

	InitializeStatsToGlobalBaseValue();

	for (const auto& Pair : StatData->BaseStats)
	{
		if (FStatDetailed* TargetStat = StatMap.Find(Pair.Key))
		{
			TargetStat->UnitBaseValue = Pair.Value;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("StatMap does not contain key %s"), *Pair.Key.ToString());	
			
			FStatDetailed NewStat;
			NewStat.UnitBaseValue = Pair.Value;
			StatMap.Add(Pair.Key, NewStat);
		}
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
		if (FStatDetailed* TargetStat = StatMap.Find(Pair.Key))
		{
			TargetStat->UnitBaseValue += Pair.Value * (Level - 1);
		}
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
				FStatDetailed NewStat;
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

int32 UUnitStatComponent::CalculateDamageAfterDefense(int32 DamageAmount, FGameplayTagContainer DamageType)
{
	return int32();
}

int32 UUnitStatComponent::CalculateDamageAfterResistance(int32 DamageAmount, FGameplayTagContainer DamageType)
{
	return int32();
}

void UUnitStatComponent::ApplyModifiers(FGameplayTag SourceTag, const TArray<FStatModifier>& Modifiers)
{

	// 스탯별로 수정자 분류
	TMap<FGameplayTag, TArray<FStatModifier>> SortedByStat;
	for (const FStatModifier& Mod : Modifiers)
	{
		SortedByStat.FindOrAdd(Mod.StatTag).Add(Mod);
	}

	for (auto& Pair : SortedByStat)
	{
		FGameplayTag TargetStatTag = Pair.Key;
		const TArray<FStatModifier>& ModList = Pair.Value;
		
		if (StatMap.Contains(TargetStatTag))
		{
			// 해당 스탯의 모디파이어 컨테이너를 만들거나 찾는다.
			FStatModifierContainer& TargetContainer = StatMap[TargetStatTag].ModifiersBySource.FindOrAdd(SourceTag);

			// 낱개 배열 보관
			TargetContainer.Modifiers = ModList;

			// 합산 연산 트리거
			TargetContainer.CalculateTotals();

			// 해당 스탯 새로고침
			StatMap[TargetStatTag].UpdateFinalValue();
		}
	}
}

void UUnitStatComponent::RemoveModifiers(FGameplayTag SourceTag)
{
	for (auto& Pair : StatMap)
	{
		if(Pair.Value.ModifiersBySource.Contains(SourceTag))
		{
			Pair.Value.ModifiersBySource.Remove(SourceTag);
			Pair.Value.UpdateFinalValue();
		}
	}
}


void UUnitStatComponent::RefreshAllStats()
{
	for (auto& Pair : StatMap)
	{
		Pair.Value.UpdateFinalValue();
	}
}

int32 UUnitStatComponent::ApplyDamage(int32 DamageAmount, const FGameplayTagContainer& DamageType)
{
	if (!GetOwner()->HasAuthority())
	{
		return 0;
	}

	int32 DamageAfterDefense = CalculateDamageAfterDefense(DamageAmount, DamageType);

	int32 FinalDamage = CalculateDamageAfterResistance(DamageAfterDefense, DamageType);


	CurrentHealth = FMath::Clamp(CurrentHealth - FinalDamage, 0, GetStat(SPTags::Stat::Combat::Primary::MaxHealth));
	if (OnHealthChanged.IsBound())
	{
		OnHealthChanged.Broadcast(CurrentHealth, FinalDamage, DamageType);
	}

	return FinalDamage;
}

int32 UUnitStatComponent::GetStat(FGameplayTag StatTag)
{
	if (const FStatDetailed* Stat = StatMap.Find(StatTag))
	{
		return Stat->FinalValue;
	}
	return 0;
}


#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Math/StatMath.h"
#include "SynchroPostTypes.generated.h"


class UItemBase;

USTRUCT(BlueprintType)
struct FStatModifier
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat Modifier")
	FGameplayTag StatTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Modifier")
	int32 FlatValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat Modifier")
	int32 PercentValue = 0;
};

USTRUCT(BlueprintType)
struct FCachedStatModifier
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 FlatValue = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 PercentValue = 0;
};

USTRUCT(BlueprintType)
struct FUnitStat
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 UnitBaseValue = 0;
	
	UPROPERTY(BlueprintReadOnly)
	FCachedStatModifier CachedModifier;

	UPROPERTY(BlueprintReadOnly)
	int32 FinalValue = 0;

	void UpdateFinalValue()
	{
		const int32 BaseWithFlat = CachedModifier.FlatValue + UnitBaseValue;
		const float Multiplier = StatMath::PercentToMultiplier(CachedModifier.PercentValue);

		FinalValue = FMath::RoundToInt(BaseWithFlat * Multiplier);
	}
};

UENUM(BlueprintType)
enum class EStatModifierSource : uint8
{
	Slot UMETA(DisplayName = "Slot"),
	Item UMETA(DisplayName = "Item"),
	Buff UMETA(DisplayName = "Buff")
};

USTRUCT(BlueprintType)
struct FPenetrationData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PhysicalFlat = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PhysicalPercent = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MagicalFlat = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MagicalPercent = 0;
};

// Damage Data Structure
USTRUCT(BlueprintType)
struct FSPDamageData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	int32 RawDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTagContainer DamageTypeTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bIsCriticalHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	AActor* DamageCauser = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FPenetrationData PenetrationData;
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common UMETA(DisplayName = "Common"),
	Uncommon UMETA(DisplayName = "Uncommon"),
	Rare UMETA(DisplayName = "Rare"),
	Epic UMETA(DisplayName = "Epic"),
	Legendary UMETA(DisplayName = "Legendary")
};


USTRUCT(BlueprintType)
struct FSlotGrowthData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Exp = 0;
};

USTRUCT(BlueprintType)
struct FSlotEconomyData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Gold = 0;
};


UENUM(BlueprintType)
enum class EFaction : uint8
{
	Player UMETA(DisplayName = "Player"),
	Enemy UMETA(DisplayName = "Enemy"),
	Neutral UMETA(DisplayName = "Neutral")
};


USTRUCT(BlueprintType)
struct FStatModifierEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FStatModifier StatModifier;

	UPROPERTY()
	TWeakObjectPtr<UObject> Source;
};
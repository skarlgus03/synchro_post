#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Math/StatMath.h"
#include "SynchroPostTypes.generated.h"


class UItemBase;
class AUnit;

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

// 한 타격 안의 피해 조각. 조각마다 방어 , 저항을 따로 계산함.
USTRUCT(BlueprintType)
struct FDamagePart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	int32 Amount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTagContainer Tags;
};

// 체력과 상호작용하는 액션에 대한 정보를 담는 구조체 ex) 데미지, 힐 등	
USTRUCT(BlueprintType)
struct FSPHealthActionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	int32 Amount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTagContainer ActionTypeTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bIsCriticalHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	AActor* Causer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FPenetrationData PenetrationData;

	// 피해 조각. 비어 있으면 Amount + ActionTypeTags를 조각 하나로 취급 (회복·DoT 등 기존 경로).
	// ApplyHealthChange의 반환값에선 조각별 최종 양이 채워지고 Amount = 합, ActionTypeTags = 태그 합집합
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TArray<FDamagePart> Parts;

	// 조각이 없으면 Amount/ActionTypeTags로 하나 만든다
	void EnsureParts()
	{
		if (Parts.Num() == 0)
		{
			FDamagePart& Part = Parts.AddDefaulted_GetRef();
			Part.Amount = Amount;
			Part.Tags = ActionTypeTags;
		}
	}

	// 조각으로 Amount(합)와 ActionTypeTags(합집합)를 다시 채운다. 합계·태그만 읽는 기존 코드 호환용
	void SyncTotalsFromParts()
	{
		Amount = 0;
		for (const FDamagePart& Part : Parts)
		{
			Amount += Part.Amount;
			ActionTypeTags.AppendTags(Part.Tags);
		}
	}
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


USTRUCT(BlueprintType)
struct FCombatEventTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Combat Event")
	FIntPoint Coordinate = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadWrite, Category = "Combat Event")
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadWrite, Category = "Combat Event")
	FSPHealthActionData ActionData;

	UPROPERTY(BlueprintReadWrite, Category = "Combat Event")
	int32 HealthBeforeChange = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Combat Event")
	int32 HealthAfterChange = 0;

	
};

// 명령 블록 사유를 나타내는 열거형
UENUM(BlueprintType)
enum class ECommandBlockReason : uint8
{
	None             UMETA(DisplayName = "사용 가능"),
	NoUnit           UMETA(DisplayName = "유닛 없음"),
	NotMyUnit        UMETA(DisplayName = "내 유닛이 아님"),
	NotThisUnitsTurn UMETA(DisplayName = "이 유닛의 턴이 아님"),
	Presenting       UMETA(DisplayName = "연출 중"), // 로컬 UI 전용. 서버 판정에 넣지 말 것
};


#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Types/SPGameplayTags.h"
#include "Types/SynchroPostTypes.h"
#include "SPSkillStructure.generated.h"

class USkillBase;
class UAnimMontage;
class UTexture2D;
class ULevelSequence;
class UStatusEffectDataAsset;

USTRUCT()
struct FSkillEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag SkillSlotTag;

	UPROPERTY()
	TObjectPtr<USkillBase> Skill;

};

USTRUCT()
struct FSkillList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSkillEntry> Entries;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FSkillEntry, FSkillList>(Entries, DeltaParams, *this);
	}

	void AddSkill(FGameplayTag SkillSlotTag, USkillBase* NewSkill)
	{
		if (NewSkill)
		{
			FSkillEntry& RealEntry = Entries.Add_GetRef(FSkillEntry());
			RealEntry.SkillSlotTag = SkillSlotTag;
			RealEntry.Skill = NewSkill;

			MarkItemDirty(RealEntry);
		}
	}

	void RemoveSkill(FGameplayTag SkillSlotTag)
	{
		Entries.RemoveAll([SkillSlotTag](const FSkillEntry& Entry) {
			return Entry.SkillSlotTag == SkillSlotTag;
		});
		MarkArrayDirty();
	}
};

template<>
struct TStructOpsTypeTraits<FSkillList> : public TStructOpsTypeTraitsBase2<FSkillList>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};


// Skill Cost Structure
USTRUCT(BlueprintType)
struct FSkillResource
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill", meta = (Categories = "Skill.Resource"))
	FGameplayTag ResourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	int32 Value = 0;

};



USTRUCT(BlueprintType)
struct FSkillTargetData
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TArray<FIntPoint> SelectedTiles;
};

UENUM(BlueprintType)
enum class ESkillTargetFaction : uint8
{
	Enemy UMETA(DisplayName = "Enemy"),
	Ally UMETA(DisplayName = "Ally"),
	Any UMETA(DisplayName = "Any"),
	None UMETA(DisplayName = "None")
};

USTRUCT(BlueprintType)
struct FSkillTargetingRule
{
	GENERATED_BODY()

	// 타일을 선택할 때, 선택 가능한 타일의 개수제한을 설정합니다. 0이면 제한 없음.
	UPROPERTY(EditAnywhere, Category = "Targeting")
	int32 RequiredTileSelectionCount = 1;


	// 어느 칸을 선택할 수 있는지 제한하는 진영
	UPROPERTY(EditAnywhere, Category = "Targeting")
	ESkillTargetFaction SelectionFaction = ESkillTargetFaction::Any;

	// 선택 칸 기준 범이 안에서 효과를 받는 진영
	UPROPERTY(EditAnywhere, Category = "Targeting")
	ESkillTargetFaction TargetFaction = ESkillTargetFaction::Any;


	// 타일을 선택했을때, 그 지점을 기준으로 퍼지는 범위 패턴을 설정합니다.
	UPROPERTY(EditAnywhere, Category = "Targeting")
	TArray<FIntPoint> RangePatternOffsets;

	// 유닛을 기준으로 스킬을 시전할 수 있는 최대 범위를 설정합니다. 0이면 제한 없음.
	UPROPERTY(EditAnywhere, Category = "Targeting")
	int32 CastRange = 1;

	// 패턴을 시전자의 방향에 맞춰 회전시킬지 여부를 설정합니다. true이면 시전자의 방향에 맞춰 패턴이 회전합니다.
	UPROPERTY(EditAnywhere, Category = "Targeting")
	bool bRotatePatternToCasterDirection = false;
};

USTRUCT(BlueprintType)
struct FSkillStatusEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<const UStatusEffectDataAsset> EffectData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1"))
	int32 Duration = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "100"))
	int32 ChancePercent = 100;
};

// Skill Data Structure
USTRUCT(BlueprintType)
struct FSkillData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FText SkillName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UTexture2D> SkillIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	int32 BaseCooldown = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TArray<FSkillResource> SkillCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FSkillTargetingRule TargetingRule;

	// 뭘 하는 스킬인지 구분하기 위한 태그. ex) 힐, 공격(유형)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Tag", meta = (Categories = "Action"))
	FGameplayTag ActionTag;

	// 피해 방식, 속성. 저항 계산이 읽는다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Tag", meta = (Categories = "Trait"))
	FGameplayTagContainer TraitTags;

	// 피해량 계산에 사용되는 계수.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage", meta = (Categories = "Stat"))
	TMap<FGameplayTag, int32> DamageCoefficients;

	// 스킬이 적용하는 상태이상 효과들. (지속시간, 확률 등)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|StatusEffect")
	TArray<FSkillStatusEffectSpec> StatusEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill" ,meta = (Categories = "Skill.StatusEffect"))
	FGameplayTagContainer BlockingTags;

	FSkillData()
	{
		BlockingTags.AddTag(SPTags::StatusEffect::CC::Silence);
		BlockingTags.AddTag(SPTags::StatusEffect::CC::Stun);
	}
	
	FGameplayTagContainer GetActionTypeTags() const
	{
		FGameplayTagContainer Tags = TraitTags;
		Tags.AddTag(ActionTag);
		return Tags;
	}
};


// 스킬이 조건을 검사하기위해서 사용하는 컨텍스트 구조체.
USTRUCT(BlueprintType)
struct FSkillExecutionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer StateTags;

	UPROPERTY(BlueprintReadWrite)
	FIntPoint CasterCoordinate;

	UPROPERTY(BlueprintReadWrite)
	EFaction CasterFaction;

	UPROPERTY(BlueprintReadWrite, meta = (Categories = "Skill.Slot"))
	FGameplayTag SkillSlotTag;
};
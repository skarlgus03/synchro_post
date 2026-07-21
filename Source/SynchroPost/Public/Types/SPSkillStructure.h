
#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Types/SPGameplayTags.h"
#include "Types/SynchroPostTypes.h"
#include "SPSkillStructure.generated.h"

class USkillBase;
class UAnimMontage;
class UTexture2D;

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


	UPROPERTY(BlueprintReadOnly, Category = "Skill")
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

	// 타일을 선택할 때, 선택 가능한 타일의 진영을 설정합니다. Enemy, Friendly, Any, None 중 하나를 선택할 수 있습니다.
	UPROPERTY(EditAnywhere, Category = "Targeting")
	ESkillTargetFaction TargetFaction = ESkillTargetFaction::Any;

	// 타일을 선택했을때, 그 지점을 기준으로 퍼지는 범위 패턴을 설정합니다.
	UPROPERTY(EditAnywhere, Category = "Targeting")
	TArray<FIntPoint> RangePatternOffsets;

	// 유닛을 기준으로 스킬을 시전할 수 있는 최대 범위를 설정합니다. 0이면 제한 없음.
	UPROPERTY(EditAnywhere, Category = "Targeting")
	int32 CastRange = 1;
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
	TObjectPtr<UAnimMontage> SkillAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FSkillTargetingRule TargetingRule;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill" ,meta = (Categories = "Skill.StatusEffect"))
	FGameplayTagContainer BlockingTags;

	FSkillData()
	{
		BlockingTags.AddTag(SPTags::StatusEffect::CC::Silence);
		BlockingTags.AddTag(SPTags::StatusEffect::CC::Stun);
	}
};


// 스킬이 조건을 검사하기위해서 사용하는 컨텍스트 구조체.
USTRUCT(BlueprintType)
struct FSkillExecutionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTagContainer StateTags;

	UPROPERTY(BlueprintReadOnly)
	FIntPoint CasterCoordinate;

	UPROPERTY(BlueprintReadOnly)
	EFaction CasterFaction;

};
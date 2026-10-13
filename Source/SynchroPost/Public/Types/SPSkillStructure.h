
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
class AActor;

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

USTRUCT(BlueprintType)
struct FAffectedTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	FIntPoint Coordinate = FIntPoint::ZeroValue;
};

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ESkillTargetFlags : uint8
{
	None = 0 UMETA(Hidden),
	Self		= 1 << 0,
	Ally		= 1 << 1, // 자신 제외 같은 진영
	Enemy		= 1 << 2, // 다른 진영 중립 포함
	Obstacle	= 1 << 3,
	EmptyTile	= 1 << 4, // 빈 칸
};
ENUM_CLASS_FLAGS(ESkillTargetFlags)

USTRUCT(BlueprintType)
struct FSkillTargetingRule
{
	GENERATED_BODY()

	// 타일을 선택할 때, 선택 가능한 타일의 개수제한을 설정합니다. 0이면 제한 없음.
	UPROPERTY(EditAnywhere, Category = "Targeting")
	int32 RequiredTileSelectionCount = 1;


	// 어느 칸을 클릭할 수 있나 (칸 위의 것과 시전자의 관계). 범위 스킬이 빈 칸을 고르려면 EmptyTile
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (Bitmask, BitmaskEnum = "/Script/SynchroPost.ESkillTargetFlags"))
	int32 SelectionMask = static_cast<int32>(ESkillTargetFlags::Enemy | ESkillTargetFlags::Obstacle);

	// 범위 안에서 누가 효과를 받나
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (Bitmask, BitmaskEnum = "/Script/SynchroPost.ESkillTargetFlags"))
	int32 TargetMask = static_cast<int32>(ESkillTargetFlags::Enemy | ESkillTargetFlags::Obstacle);


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

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer StateTags;

	UPROPERTY(BlueprintReadWrite)
	FIntPoint CasterCoordinate = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadWrite)
	EFaction CasterFaction = EFaction::Neutral;

	UPROPERTY(BlueprintReadWrite, meta = (Categories = "Skill.Slot"))
	FGameplayTag SkillSlotTag;

	UPROPERTY(BlueprintReadWrite)
	FIntPoint AimCoordinate = FIntPoint::ZeroValue;
};
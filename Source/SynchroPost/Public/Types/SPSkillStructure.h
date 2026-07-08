
#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayTagContainer.h"
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
	int32 TargetCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UAnimMontage> SkillAnimation;

};

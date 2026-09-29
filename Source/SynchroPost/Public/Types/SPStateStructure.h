#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayTagContainer.h"
#include "SPStateStructure.generated.h"

class UStatusEffectBase;
class UStatusEffectDataAsset;

namespace StatusEffectConst
{
    constexpr int32 Infinite = -1;
}

UENUM(BlueprintType)
enum class EStackingPolicy : uint8
{
    Independent UMETA(DisplayName = "개별 적용"),
    RefreshDuration UMETA(DisplayName = "지속시간 덮어 씀"),
    StackCounter UMETA(DisplayName = "스택 카운터"),
};


USTRUCT(BlueprintType)
struct FStateTagEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "State")
	TObjectPtr<const UStatusEffectDataAsset> DataAsset;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    int32 RemainingDuration = StatusEffectConst::Infinite;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    int32 StackCount = 1; 

    UPROPERTY(NotReplicated)
    TObjectPtr<UStatusEffectBase> EffectInstance;

    FStateTagEntry() {}
    FStateTagEntry(const UStatusEffectDataAsset* InData, int32 InDuration, UStatusEffectBase* InEffect)
        : DataAsset(InData), RemainingDuration(InDuration), EffectInstance(InEffect) {}

    FGameplayTag GetTag() const;
};


USTRUCT(BlueprintType)
struct FStateTagList : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FStateTagEntry> Entries;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FStateTagEntry>(Entries, DeltaParms, *this);
    }

    // 태그로 하나 찾기 (RefreshDuration/StackCounter 용 - Independent는 여러 개 있을 수 있어 이 함수로 못 찾음)
    FStateTagEntry* FindFirst(const FGameplayTag& Tag)
    {
        return Entries.FindByPredicate(
            [&Tag](const FStateTagEntry& Entry) { return Entry.GetTag() == Tag; });
    }

    const FStateTagEntry* FindFirst(const FGameplayTag& Tag) const
    {
        return Entries.FindByPredicate(
            [&Tag](const FStateTagEntry& Entry) { return Entry.GetTag() == Tag; });
	}

    // 새 엔트리를 추가한다. 
    void Add(const FStateTagEntry& NewEntry)
    {
        Entries.Add(NewEntry);
        MarkItemDirty(Entries.Last());
	}

    // RefreshDuration 정책, 더 긴 지속시간으로 덮어쓴다.
    void RefreshDuration(FStateTagEntry& Existing, int32 NewDuration)
    {
        Existing.RemainingDuration = MaxDuration(Existing.RemainingDuration, NewDuration);
		MarkItemDirty(Existing);
    }

    // StackCounter정책, 스택 수를 +1 하고(최대스택은 못넘어) 지속시간은 더 긴쪽으로 덮어씀. 
    void IncrementStack(FStateTagEntry& Existing, int32 Duration, int32 MaxStack)
    {
		Existing.StackCount = (MaxStack == StatusEffectConst::Infinite)
			? Existing.StackCount + 1
            : FMath::Min(Existing.StackCount + 1, MaxStack);
		Existing.RemainingDuration = MaxDuration(Existing.RemainingDuration, Duration);
        MarkItemDirty(Existing);
    }


    bool RemoveByInstance(UStatusEffectBase* Instance) 
    {
        int32 RemovedCount = Entries.RemoveAll(
            [Instance](const FStateTagEntry& Entry) { return Entry.EffectInstance == Instance; });

        if (RemovedCount > 0)
        {
            MarkArrayDirty();
            return true;
        }
        return false;
    }

	// 모든 엔트리의 RemainingDuration을 1 감소시키고, 0이 된 엔트리를 반환
    TArray<FStateTagEntry> ReduceDurationsAndGetExpired()
    {
        TArray<FStateTagEntry> Expired;

        for (FStateTagEntry& Entry : Entries)
        {
            if (Entry.RemainingDuration > 0)
            {
                Entry.RemainingDuration--;
                MarkItemDirty(Entry);

                if (Entry.RemainingDuration == 0)
                {
                    Expired.Add(Entry);
                }
            }
        }

        return Expired;
    }

private:

	// 두 지속시간 중 더 긴 것을 반환, 무한이면 무한 반환
    static int32 MaxDuration(int32 A, int32 B)
    {
        if (A == StatusEffectConst::Infinite || B == StatusEffectConst::Infinite)
        {
            return StatusEffectConst::Infinite;
		}
		return FMath::Max(A, B);
    }
};

template<>
struct TStructOpsTypeTraits<FStateTagList> : public TStructOpsTypeTraitsBase2<FStateTagList>
{
    enum { WithNetDeltaSerializer = true };
};

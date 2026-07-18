#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayTagContainer.h"
#include "SPStateTypes.generated.h"

class UStatusEffectBase;

USTRUCT(BlueprintType)
struct FStateTagEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State", meta = (Categories = "State"))
    FGameplayTag StateTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
    int32 RemainingDuration = -1;

    UPROPERTY(BlueprintReadOnly, Category = "State")
    TObjectPtr<UStatusEffectBase> EffectInstance;

    FStateTagEntry() {}
    FStateTagEntry(const FGameplayTag& InTag, int32 InDuration, UStatusEffect* InEffect = nullptr)
        : StateTag(InTag), RemainingDuration(InDuration), EffectInstance(InEffect) {}
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

    FStateTagEntry* Find(const FGameplayTag& Tag)
    {
        return Entries.FindByPredicate(
            [&Tag](const FStateTagEntry& Entry) { return Entry.StateTag == Tag; }
        );
    }

    const FStateTagEntry* Find(const FGameplayTag& Tag) const
    {
        return Entries.FindByPredicate(
            [&Tag](const FStateTagEntry& Entry) { return Entry.StateTag == Tag; }
        );
	}

	// true : 새로운 태그 추가, false: 기존 태그 갱신
    bool AddOrUpdate(const FGameplayTag& Tag, int32 Duration, UStatusEffectBase* EffectInstance = nullptr)
    {
        if (FStateTagEntry* Existing = Find(Tag))
        {
            Existing->RemainingDuration = FMath::Max(Existing->RemainingDuration, Duration);
            MarkItemDirty(*Existing);
            return false;
        }

        FStateTagEntry NewEntry(Tag, Duration, EffectInstance);
        Entries.Add(NewEntry);
        MarkItemDirty(Entries.Last());
        return true;
    }

    bool Remove(const FGameplayTag& Tag)
    {
        int32 RemovedCount = Entries.RemoveAll(
            [&Tag](const FStateTagEntry& Entry) { return Entry.StateTag == Tag; });

        if (RemovedCount > 0)
        {
            MarkArrayDirty();
            return true;
        }
        return false;
    }

    // 0이 된 태그들을 반환함
    TArray<FGameplayTag> ReduceDurations()
    {
        TArray<FGameplayTag> ExpiredTags;

        for (FStateTagEntry& Entry : Entries)
        {
            if (Entry.RemainingDuration > 0)
            {
                Entry.RemainingDuration--;
                MarkItemDirty(Entry);

                if (Entry.RemainingDuration == 0)
                {
                    ExpiredTags.Add(Entry.StateTag);
                }
            }
        }

        
        return ExpiredTags;
    }
};

template<>
struct TStructOpsTypeTraits<FStateTagList> : public TStructOpsTypeTraitsBase2<FStateTagList>
{
    enum { WithNetDeltaSerializer = true };
};
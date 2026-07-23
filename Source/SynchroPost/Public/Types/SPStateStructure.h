#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GameplayTagContainer.h"
#include "SPStateStructure.generated.h"

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
    int32 StackCount = 1; 

    UPROPERTY(BlueprintReadOnly, Category = "State")
    TObjectPtr<UStatusEffectBase> EffectInstance;

    FStateTagEntry() {}
    FStateTagEntry(const FGameplayTag& InTag, int32 InDuration, UStatusEffectBase* InEffect = nullptr)
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

    // 태그로 하나 찾기 (RefreshDuration/StackCounter 용 - Independent는 여러 개 있을 수 있어 이 함수로 못 찾음)
    FStateTagEntry* FindFirst(const FGameplayTag& Tag)
    {
        return Entries.FindByPredicate(
            [&Tag](const FStateTagEntry& Entry) { return Entry.StateTag == Tag; });
    }

    const FStateTagEntry* FindFirst(const FGameplayTag& Tag) const
    {
        return Entries.FindByPredicate(
            [&Tag](const FStateTagEntry& Entry) { return Entry.StateTag == Tag; });
	}

    // Independent 정책 - 무조건 새 엔트리 추가
    void AddIndependent(const FGameplayTag& Tag, int32 Duration, UStatusEffectBase* EffectInstance)
    {
        Entries.Add(FStateTagEntry(Tag, Duration, EffectInstance));
        MarkItemDirty(Entries.Last());
    }

    // RefreshDuration 정책 - 있으면 지속시간만 갱신, 없으면 새로 추가. 반환값: 새로 만든 EffectInstance를 실제 썼는지
    bool AddOrRefresh(const FGameplayTag& Tag, int32 Duration, UStatusEffectBase* EffectInstance)
    {
        if (FStateTagEntry* Existing = FindFirst(Tag))
        {
            Existing->RemainingDuration = FMath::Max(Existing->RemainingDuration, Duration);
            MarkItemDirty(*Existing);
            return false; // 기존 유지, 새 인스턴스는 버려져야 함
        }

        Entries.Add(FStateTagEntry(Tag, Duration, EffectInstance));
        MarkItemDirty(Entries.Last());
        return true;
    }

    // StackCounter 정책 - 있으면 스택만 증가(Max 캡), 없으면 새로 추가
    bool AddOrIncrementStack(const FGameplayTag& Tag, int32 Duration, int32 MaxStack, UStatusEffectBase* EffectInstance)
    {
        if (FStateTagEntry* Existing = FindFirst(Tag))
        {
            Existing->StackCount = FMath::Min(Existing->StackCount + 1, MaxStack);
            Existing->RemainingDuration = FMath::Max(Existing->RemainingDuration, Duration);
            MarkItemDirty(*Existing);
            return false;
        }

        Entries.Add(FStateTagEntry(Tag, Duration, EffectInstance));
        MarkItemDirty(Entries.Last());
        return true;
    }

    bool Remove(const FStateTagEntry& TargetEntry) // 정확히 이 인스턴스를 지정해서 제거 (Independent라 동일 태그가 여럿일 수 있으므로)
    {
        int32 RemovedCount = Entries.RemoveAll(
            [&TargetEntry](const FStateTagEntry& Entry) { return Entry.EffectInstance == TargetEntry.EffectInstance; });

        if (RemovedCount > 0)
        {
            MarkArrayDirty();
            return true;
        }
        return false;
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
};

template<>
struct TStructOpsTypeTraits<FStateTagList> : public TStructOpsTypeTraitsBase2<FStateTagList>
{
    enum { WithNetDeltaSerializer = true };
};

UENUM(BlueprintType)
enum class EStackingPolicy : uint8
{
	Independent UMETA(DisplayName = "Independent"),
	RefreshDuration UMETA(DisplayName = "RefreshDuration"),
	StackCounter UMETA(DisplayName = "StackCounter"),
};
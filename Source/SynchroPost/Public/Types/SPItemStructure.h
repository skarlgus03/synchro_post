
#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Item/ItemBase.h"
#include "GameplayTagContainer.h"
#include "SPItemStructure.generated.h"


struct FInventoryList;
struct FEquippedItemList;

USTRUCT(BlueprintType)
struct FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UItemBase> Item = nullptr;

	void PostReplicatedAdd(const FFastArraySerializer& InArraySerializer)
	{
		if (Item)
		{
			UE_LOG(LogTemp, Log, TEXT("Inventory item is added: %s"), *Item->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Inventory item is added but Item is null!"));
		}
	}
	void PostReplicatedChange(const FFastArraySerializer& InArraySerializer)
	{
		if (Item)
		{
			UE_LOG(LogTemp, Log, TEXT("Inventory item is changed: %s"), *Item->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Inventory item is changed but Item is null!"));
		}
	}
	void PreReplicatedRemove(const FFastArraySerializer& InArraySerializer)
	{
		UE_LOG(LogTemp, Log, TEXT("Inventory item is about to be removed: %s"), *Item->GetName());
	}
}; 

USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInventoryEntry> Entries;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams) 
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(Entries, DeltaParams, *this);
	}

	void AddItem(UItemBase* NewItem)
	{
		if (NewItem)
		{
			FInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
			NewEntry.Item = NewItem;
			MarkItemDirty(NewEntry);
		}
	}

	void RemoveItem(UItemBase* Item)
	{
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			if (Entries[i].Item == Item)
			{
				Entries.RemoveAt(i);
				MarkArrayDirty();
				return;
			}
		}
	}
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

USTRUCT(BlueprintType)
struct FEquippedItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag SlotTag;

	UPROPERTY()
	TObjectPtr<UItemBase> Item = nullptr;

	UPROPERTY()
	int32 SlotIndex = 0;

	void PostReplicatedAdd(const FFastArraySerializer& InArraySerializer)
	{
		if (Item)
		{
			UE_LOG(LogTemp, Log, TEXT("Equipped item is added: %s in slot %s"), *Item->GetName(), *SlotTag.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Equipped item is added but Item is null in slot %s!"), *SlotTag.ToString());
		}
	}
	void PostReplicatedChange(const FFastArraySerializer& InArraySerializer)
	{
		if (Item)
		{
			UE_LOG(LogTemp, Log, TEXT("Equipped item is changed: %s in slot %s"), *Item->GetName(), *SlotTag.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Equipped item is changed but Item is null in slot %s!"), *SlotTag.ToString());
		}
	}
	void PreReplicatedRemove(const FFastArraySerializer& InArraySerializer)
	{
		UE_LOG(LogTemp, Log, TEXT("Equipped item is about to be removed: %s from slot %s"), *Item->GetName(), *SlotTag.ToString());
	}
};

USTRUCT(BlueprintType)
struct FEquippedItemList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FEquippedItemEntry> Entries;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams) 
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FEquippedItemEntry, FEquippedItemList>(Entries, DeltaParams, *this);
	}

	void AddItem(FGameplayTag SlotTag, UItemBase* NewItem, int32 SlotIndex = 0)
	{
		if (NewItem)
		{
			for (FEquippedItemEntry& Entry : Entries)
			{
				if (Entry.SlotTag == SlotTag && Entry.SlotIndex == SlotIndex)
				{
					Entry.Item = NewItem;
					MarkItemDirty(Entry);
					return;
				}
			}

			FEquippedItemEntry& NewEntry = Entries.AddDefaulted_GetRef();
			NewEntry.SlotTag = SlotTag;
			NewEntry.Item = NewItem;
			MarkItemDirty(NewEntry);
		}
	}

	void RemoveItem(const FGameplayTag& SlotTag , int32 SlotIndex = 0)
	{
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			if (Entries[i].SlotTag == SlotTag && Entries[i].SlotIndex == SlotIndex)
			{

				Entries[i].Item = nullptr;
				Entries.RemoveAt(i);
				MarkArrayDirty();
				return;
			}
		}
	}

};

template<>
struct TStructOpsTypeTraits<FEquippedItemList> : public TStructOpsTypeTraitsBase2<FEquippedItemList>
{
	enum { WithNetDeltaSerializer = true };
};
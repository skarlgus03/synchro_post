#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SynchroPostTypes.h"
#include "Types/SPItemStructures.h"
#include "UnitSlot.generated.h"

class AUnit;
class UItemDataAsset;
class UItemBase;
class UActorChannel;

UCLASS()
class SYNCHROPOST_API UUnitSlot : public UObject
{
	GENERATED_BODY()
	
public:

	UUnitSlot();
	
	
	// Item Fucntions

	UFUNCTION(BlueprintCallable, Category = "Slot")
	void EquipItemFromInventoryIndex(UItemBase* ItemBase, int32 SlotIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Slot")
	void EquipItemDirect(UItemBase* ItemBase);


	UFUNCTION(BlueprintCallable, Category = "Slot")
	void UnequipItemToInventory(FGameplayTag SlotTag, int32 SlotIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Slot")
	void AddItemToInventory(UItemBase* NewItem);


	
	UFUNCTION(BlueprintCallable, Category = "Slot")
	void SetUnit(AUnit* NewUnit);


	// 장착아이템 리스트를 훑어 스탯 모디파이어를 재계산한다.
	void RefreshEquipmentStatModifiers();

	// 장착 아이템 스탯모디파이어를 스탯 컴포넌트에게 전달한다.
	void PushStatModifiersToStatComponent();

	// Replication
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsSupportedForNetworking() const override { return true; }

	UItemBase* GetEquippedItemByTag(FGameplayTag SlotTag, int32 SlotIndex = 0) const
	{
		for (const FEquippedItemEntry& Entry : EquippedItem.Entries)
		{
			if (Entry.SlotTag == SlotTag && Entry.SlotIndex == SlotIndex)
			{
				return Entry.Item;
			}
		}
		return nullptr;
	}



	
protected:

	// The current unit occupying this slot
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Slot|Unit")
	TObjectPtr<AUnit> CurrentUnit;

	

	UPROPERTY()
	TArray<FStatModifierEntry> EquipmentStatModifiers;

	UPROPERTY()
	TArray<FStatModifierEntry> SlotBonusStatModifiers;






	// Slot Growth Datas
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Slot|Growth")
	FSlotGrowthData SlotGrowthData;

	// Slot Economy Datas
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Slot|Economy")
	FSlotEconomyData SlotEconomyData;


	// The inventory of items in this slot
	UPROPERTY(Replicated)
	FInventoryList SlotInventory;

	UPROPERTY(Replicated, ReplicatedUsing=OnRep_EquippedItem)
	FEquippedItemList EquippedItem;

protected:

	UFUNCTION()
	void OnRep_EquippedItem();

private:

	bool HasAuthorityFromOuter() const;

};

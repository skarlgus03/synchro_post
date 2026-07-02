#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SynchroPostTypes.h"
#include "Types/SPItemStructures.h"
#include "UnitSlot.generated.h"

class AUnit;
class UItemDataAsset;
class UItemInstance;
class UActorChannel;

UCLASS()
class SYNCHROPOST_API UUnitSlot : public UObject
{
	GENERATED_BODY()
	
public:

	UUnitSlot();
	
	
	

	UFUNCTION(BlueprintCallable, Category = "Slot")
	void EquipItemFromInventoryIndex(UItemInstance* ItemInstance, int32 SlotIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Slot")
	void EquipItemDirect(UItemInstance* ItemInstance);


	UFUNCTION(BlueprintCallable, Category = "Slot")
	void UnequipItemToInventory(FGameplayTag SlotTag, int32 SlotIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Slot")
	void AddItemToInventory(UItemInstance* NewItem);
	
	UFUNCTION(BlueprintCallable, Category = "Slot")
	void SetUnit(AUnit* NewUnit);

	void UpdateSlotFinalStats();



	// Replication
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsSupportedForNetworking() const override { return true; }

	UItemInstance* GetEquippedItemByTag(FGameplayTag SlotTag, int32 SlotIndex = 0) const
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

	// Slot Bonus Stat Modifiers
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Slot|Stats")
	TMap<FGameplayTag, FStatModifier> StatModifiers;


	// Slot Growth Datas
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot|Growth")
	int32 SlotLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot|Growth")
	int32 SlotExp = 0;

	// Slot Economy Datas
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot|Economy")
	int32 SlotGold = 0;
		

	// The inventory of items in this slot

	UPROPERTY(Replicated)
	FInventoryList SlotInventory;

	UPROPERTY(Replicated)
	FEquippedItemList EquippedItem;

protected:


private:

	bool HasAuthorityFromOuter() const;

};

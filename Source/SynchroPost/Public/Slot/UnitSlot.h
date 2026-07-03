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
	
	
	// Item Fucntions

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

	UFUNCTION(BlueprintCallable, Category = "Slot")
	void CacheStatModifiers();


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



	// Getter

	const TMap<FGameplayTag, FCachedStatModifier>& GetCachedStatModifiers() const { return CachedStatModifiers; }

protected:

	// The current unit occupying this slot
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Slot|Unit")
	TObjectPtr<AUnit> CurrentUnit;

	// Slot Bonus Stat Modifiers
	UPROPERTY(BlueprintReadOnly, Category = "Slot|Stats")
	TMap<FGameplayTag, FCachedStatModifier> CachedStatModifiers;


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

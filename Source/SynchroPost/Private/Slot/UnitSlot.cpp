
#include "Slot/UnitSlot.h"
#include "Unit/Unit.h"
#include "Item/ItemDataAsset.h"
#include "Item/ItemInstance.h"


UUnitSlot::UUnitSlot()
{
}

void UUnitSlot::EquipItem(UItemInstance* ItemInstance)
{
	if (!ItemInstance || !ItemInstance->GetItemDataAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItem called with null ItemInstance or ItemDataAsset"));
		return;
	}
	
	FGameplayTag TargetEquipmentTag = ItemInstance->GetItemDataAsset()->EquipmentTag;

	if (EquippedItems.Contains(TargetEquipmentTag))
	{
		UnequipItem(TargetEquipmentTag);
	}

	SlotInventory.Remove(ItemInstance);

	EquippedItems.Add(TargetEquipmentTag, ItemInstance);

	if (CurrentUnit)
	{
		ItemInstance->OnEquipped(this, CurrentUnit);
	}

	UpdateSlotFinalStats();

	UE_LOG(LogTemp, Log, TEXT("Equipped item with tag: %s. Item Name : %s"), *TargetEquipmentTag.ToString(), *ItemInstance->GetName());
}

void UUnitSlot::UnequipItem(FGameplayTag EquipmentTag)
{
	TObjectPtr<UItemInstance>* FoundItemPtr = EquippedItems.Find(EquipmentTag);
	if (!FoundItemPtr || !(*FoundItemPtr))
	{
		return;
	}

	TObjectPtr<UItemInstance> ItemToUnequip = *FoundItemPtr;

	EquippedItems.Remove(EquipmentTag);
	UpdateSlotFinalStats();
	ItemToUnequip->OnUnequipped();
	SlotInventory.Add(ItemToUnequip);

	UE_LOG(LogTemp, Log, TEXT("Unequipped item with tag: %s. Item Name : %s"), *EquipmentTag.ToString(), *ItemToUnequip->GetName());

}

void UUnitSlot::SetUnit(AUnit* NewUnit)
{
}

void UUnitSlot::UpdateSlotFinalStats()
{
}

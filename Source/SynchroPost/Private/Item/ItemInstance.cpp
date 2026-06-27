#include "Item/ItemInstance.h"

void UItemInstance::OnEquipped_Implementation(UUnitSlot* Slot, AUnit* TargetUnit)
{
	EquippedSlot = Slot;
	EquippedUnit = TargetUnit;
}

void UItemInstance::OnUnequipped_Implementation()
{
	EquippedSlot = nullptr;
	EquippedUnit = nullptr;
}

void UItemInstance::InitializeItem(UItemDataAsset* NewItemData)
{
	if (!NewItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeItem called with null NewItemData"));
		return;
	}
	ItemData = NewItemData;
}

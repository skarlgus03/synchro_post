#include "Item/ItemInstance.h"

#include "Net/UnrealNetwork.h"

void UItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemInstance, UpgradeLevel);
}

void UItemInstance::OnEquipped_Implementation(UUnitSlot* Slot, AUnit* TargetUnit)
{
	OwnerSlot = Slot;
	EquippedUnit = TargetUnit;
}

void UItemInstance::OnUnequipped_Implementation()
{
	OwnerSlot = nullptr;
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

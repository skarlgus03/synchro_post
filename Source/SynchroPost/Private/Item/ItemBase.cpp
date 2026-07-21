#include "Item/ItemBase.h"

#include "Net/UnrealNetwork.h"

void UItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemBase, UpgradeLevel);
}

void UItemBase::OnEquipped_Implementation(UUnitSlot* Slot, AUnit* TargetUnit)
{
	OwnerSlot = Slot;
	EquippedUnit = TargetUnit;
}

void UItemBase::OnUnequipped_Implementation()
{
	OwnerSlot = nullptr;
	EquippedUnit = nullptr;
}

TArray<FStatModifierEntry> UItemBase::GetStatModifierEntries() const
{
	TArray<FStatModifierEntry> Entries;

	if (ItemData)
	{
		for (const FStatModifier& Modifier : ItemData->ItemStatModifiers)
		{
			FStatModifierEntry Entry;
			Entry.StatModifier = Modifier;
			Entry.Source = const_cast<UItemBase*>(this);
			Entries.Add(Entry);
		}
		return Entries;
	}

	return Entries;
}

void UItemBase::InitializeItem(UItemDataAsset* NewItemData)
{
	if (!NewItemData)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeItem called with null NewItemData"));
		return;
	}
	ItemData = NewItemData;
}

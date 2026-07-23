
#include "Slot/UnitSlot.h"
#include "Unit/Unit.h"
#include "Item/ItemDataAsset.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "Item/ItemBase.h"
#include "Unit/StatComponent.h"

UUnitSlot::UUnitSlot()
{

}

void UUnitSlot::EquipItemFromInventoryIndex(UItemBase* ItemBase, int32 SlotIndex)
{
	if (!HasAuthorityFromOuter())
	{
		return;
	}
	if (!ItemBase || !ItemBase->GetItemDataAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItemFromInventory called with null ItemBase or ItemDataAsset"));
		return;
	}

	bool bFoundInInventory = false;

	for (const FInventoryEntry& Entry : SlotInventory.Entries)
	{
		if (Entry.Item == ItemBase)
		{
			bFoundInInventory = true;
			break;
		}
	}
	if (!bFoundInInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item is not existed in Inventory"));
		return;
	}

	FGameplayTag ItemTag = ItemBase->GetItemDataAsset()->EquipmentTag;

	for (const FEquippedItemEntry& Entry : EquippedItem.Entries)
	{
		if (Entry.SlotTag == ItemTag && Entry.SlotIndex == SlotIndex && Entry.Item != nullptr)
		{
			UnequipItemToInventory(ItemTag, SlotIndex);
			break;
		}
	}

	SlotInventory.RemoveItem(ItemBase);
	EquippedItem.AddItem(ItemTag, ItemBase, SlotIndex);

	RefreshEquipmentStatModifiers();
}

void UUnitSlot::EquipItemDirect(UItemBase* ItemBase)
{
	if (!HasAuthorityFromOuter())
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItemDirect called on non-authority object"));
		return;
	}

	if (!ItemBase || !ItemBase->GetItemDataAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItemDirect called with null ItemBase or ItemDataAsset"));
		return;
	}

	AddItemToInventory(ItemBase);
	EquipItemFromInventoryIndex(ItemBase);
}

void UUnitSlot::UnequipItemToInventory(FGameplayTag SlotTag, int32 SlotIndex)
{
	if (!HasAuthorityFromOuter())
	{
		UE_LOG(LogTemp, Warning, TEXT("UnequipItem called on non-authority object"));
		return;
	}

	if (!SlotTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UnequipItem called with invalid SlotTag"));
		return;
	}

	UItemBase* ItemToUnequip = nullptr;
	for (const FEquippedItemEntry& Entry : EquippedItem.Entries)
	{
		if (Entry.SlotTag == SlotTag && Entry.SlotIndex == SlotIndex)
		{
			ItemToUnequip = Entry.Item;
			break;
		}
	}

	if (!ItemToUnequip)
	{
		UE_LOG(LogTemp, Warning, TEXT("No item equipped in slot with tag: %s"), *SlotTag.ToString());
		return;
	}

	EquippedItem.RemoveItem(SlotTag, SlotIndex);
	SlotInventory.AddItem(ItemToUnequip);

	RefreshEquipmentStatModifiers();
}

void UUnitSlot::AddItemToInventory(UItemBase* NewItem)
{
	if (!HasAuthorityFromOuter())
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItemToInventory called on non-authority object"));
		return;
	}

	if (!NewItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItemToInventory called with null NewItem"));
		return;
	}

	SlotInventory.AddItem(NewItem);
	UE_LOG(LogTemp, Log, TEXT("Added item to inventory: %s"), *NewItem->GetName());

}

void UUnitSlot::SetUnit(AUnit* NewUnit)
{
	if (!HasAuthorityFromOuter())
	{
		UE_LOG(LogTemp, Warning, TEXT("SetUnit called on non-authority object"));
		return;
	}
	if (!NewUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetUnit called with null NewUnit"));
		return;
	}
	if (CurrentUnit == NewUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetUnit called with the same unit: %s"), *NewUnit->GetName());
		return;
	}

	CurrentUnit = NewUnit;

	if (CurrentUnit)
	{
		CurrentUnit->SetCurrentSlot(this);
	}

	RefreshEquipmentStatModifiers();
}

void UUnitSlot::RefreshEquipmentStatModifiers()
{
	EquipmentStatModifiers.Reset();

	for (const FEquippedItemEntry& Entry : EquippedItem.Entries)
	{
		if (Entry.Item)
		{
			EquipmentStatModifiers.Append(Entry.Item->GetStatModifierEntries());
		}
	}

	PushStatModifiersToStatComponent();
}

void UUnitSlot::PushStatModifiersToStatComponent()
{
	if (!CurrentUnit)
	{
		return;
	}
	if (!CurrentUnit->GetStatComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("CurrentUnit has no StatComponent"));
		return;
	}
	TArray<FStatModifierEntry> NewModifiers;
	NewModifiers.Append(SlotBonusStatModifiers);
	NewModifiers.Append(EquipmentStatModifiers);

	CurrentUnit->GetStatComponent()->SetSlotModifiers(NewModifiers);

}

void UUnitSlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UUnitSlot, SlotInventory);
	DOREPLIFETIME(UUnitSlot, EquippedItem);
	DOREPLIFETIME(UUnitSlot, CurrentUnit);
	DOREPLIFETIME(UUnitSlot, SlotGrowthData);
	DOREPLIFETIME(UUnitSlot, SlotEconomyData)
}



void UUnitSlot::OnRep_EquippedItem()
{
	RefreshEquipmentStatModifiers();
}

bool UUnitSlot::HasAuthorityFromOuter() const
{
	UObject* OuterObj = GetOuter();
	if (!OuterObj)
	{
		return false;
	}

	if (UActorComponent* OuterComp = Cast<UActorComponent>(OuterObj))
	{
		if (AActor* OwnerActor = OuterComp->GetOwner())
		{
			return OwnerActor->HasAuthority();
		}
	}
	else if (AActor* OuterActor = Cast<AActor>(OuterObj))
	{
		return OuterActor->HasAuthority();
	}
	return false;
}

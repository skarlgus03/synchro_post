
#include "Slot/UnitSlot.h"
#include "Unit/Unit.h"
#include "Item/ItemDataAsset.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "Item/ItemInstance.h"

UUnitSlot::UUnitSlot()
{

}

void UUnitSlot::EquipItemFromInventoryIndex(UItemInstance* ItemInstance, int32 SlotIndex)
{
	if (!HasAuthorityFromOuter())
	{
		return;
	}
	if (!ItemInstance || !ItemInstance->GetItemDataAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItemFromInventory called with null ItemInstance or ItemDataAsset"));
		return;
	}

	bool bFoundInInventory = false;

	for (const FInventoryEntry& Entry : SlotInventory.Entries)
	{
		if (Entry.Item == ItemInstance)
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

	FGameplayTag ItemTag = ItemInstance->GetItemDataAsset()->EquipmentTag;

	for (const FEquippedItemEntry& Entry : EquippedItem.Entries)
	{
		if (Entry.SlotTag == ItemTag && Entry.SlotIndex == SlotIndex && Entry.Item != nullptr)
		{
			UnequipItemToInventory(ItemTag, SlotIndex);
			break;
		}
	}

	SlotInventory.RemoveItem(ItemInstance);
	EquippedItem.AddItem(ItemTag, ItemInstance, SlotIndex);

	CacheStatModifiers();
	UE_LOG(LogTemp, Log, TEXT("Equipped item with tag: %s at index: %d. Item Name : %s"), *ItemTag.ToString(), SlotIndex, *ItemInstance->GetName());
}

void UUnitSlot::EquipItemDirect(UItemInstance* ItemInstance)
{
	if (!HasAuthorityFromOuter())
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItemDirect called on non-authority object"));
		return;
	}

	if (!ItemInstance || !ItemInstance->GetItemDataAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItemDirect called with null ItemInstance or ItemDataAsset"));
		return;
	}

	AddItemToInventory(ItemInstance);
	EquipItemFromInventoryIndex(ItemInstance);
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

	UItemInstance* ItemToUnequip = nullptr;
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

	CacheStatModifiers();
	UE_LOG(LogTemp, Log, TEXT("Unequipped item with tag: %s. Item Name : %s"), *SlotTag.ToString(), *ItemToUnequip->GetName());
}

void UUnitSlot::AddItemToInventory(UItemInstance* NewItem)
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

	CacheStatModifiers();
}


void UUnitSlot::CacheStatModifiers()
{
	CachedStatModifiers.Empty();

	// Cache Item's Stat Modifiers

	for (const FEquippedItemEntry& Entry : EquippedItem.Entries)
	{
		if (Entry.Item && Entry.Item->GetItemDataAsset())
		{
			const TArray<FStatModifier>& ItemModifiers = Entry.Item->GetItemDataAsset()->ItemStatModifiers;

			for (const FStatModifier& Modifier : ItemModifiers)
			{
				if (CachedStatModifiers.Contains(Modifier.StatTag))
				{
					CachedStatModifiers[Modifier.StatTag].FlatValue += Modifier.FlatValue;
					CachedStatModifiers[Modifier.StatTag].PercentValue += Modifier.PercentValue;
				}
				else
				{
					CachedStatModifiers.Add(Modifier.StatTag, FCachedStatModifier{ Modifier.FlatValue, Modifier.PercentValue });
				}
			}
		}
	}

	// Cache Slot's Bonus Stat. (Not Yet)
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
	CacheStatModifiers();
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

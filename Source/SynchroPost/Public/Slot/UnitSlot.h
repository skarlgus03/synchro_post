#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SynchroPostTypes.h"
#include "UnitSlot.generated.h"

class AUnit;
class UItemDataAsset;
class UItemInstance;

UCLASS()
class SYNCHROPOST_API UUnitSlot : public UObject
{
	GENERATED_BODY()
	
public:

	UUnitSlot();
	
	
	
	UFUNCTION(BlueprintCallable, Category = "Slot")
	void EquipItem(UItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category = "Slot")
	void UnequipItem(FGameplayTag EquipmentTag);
	
	UFUNCTION(BlueprintCallable, Category = "Slot")
	void SetUnit(AUnit* NewUnit);

	void UpdateSlotFinalStats();


protected:

	// The current unit occupying this slot
	UPROPERTY(BlueprintReadOnly, Category = "Slot|Unit")
	TObjectPtr<AUnit> CurrentUnit;

	// Slot Bonus Stat Modifiers
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot|Stats")
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot|Item")
	TArray<TObjectPtr<UItemInstance>> SlotInventory;

	// Slot's currently equipped items
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slot|Item")
	TMap<FGameplayTag, TObjectPtr<UItemInstance>> EquippedItems;
};

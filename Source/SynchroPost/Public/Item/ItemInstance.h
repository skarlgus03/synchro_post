#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Item/ItemDataAsset.h"
#include "ItemInstance.generated.h"

class UUnitSlot;
class AUnit;

UCLASS()
class SYNCHROPOST_API UItemInstance : public UObject
{
	GENERATED_BODY()
	


public:

	UFUNCTION(BlueprintNativeEvent, Category = "Item")
	void OnEquipped(UUnitSlot* Slot, AUnit* TargetUnit);

	virtual void OnEquipped_Implementation(UUnitSlot* Slot, AUnit* TargetUnit);

	UFUNCTION(BlueprintNativeEvent, Category = "Item")
	void OnUnequipped();
	
	virtual void OnUnequipped_Implementation();


	UFUNCTION(BlueprintCallable, Category = "Item")
	UItemDataAsset* GetItemData() const { return ItemData; }

	UFUNCTION(BlueprintCallable, Category = "Item")
	TArray<FStatModifier> GetItemStatModifiers() const { return ItemData ? ItemData->ItemStatModifiers : TArray<FStatModifier>(); }
	
	void InitializeItem(UItemDataAsset* NewItemData);

	UItemDataAsset* GetItemDataAsset() const { return ItemData; }

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly , Category = "Item")
	TObjectPtr<UItemDataAsset> ItemData;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<UUnitSlot> EquippedSlot;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<AUnit> EquippedUnit;
};
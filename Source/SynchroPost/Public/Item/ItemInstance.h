#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Item/ItemDataAsset.h"
#include "ItemInstance.generated.h"

class UUnitSlotComponent;
class AUnit;

UCLASS()
class SYNCHROPOST_API UItemInstance : public UObject
{
	GENERATED_BODY()
	

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UItemDataAsset> ItemData;


public:

	UFUNCTION(BlueprintNativeEvent, Category = "Item")
	void OnEquipped(UUnitSlotComponent* SlotComponent, AUnit* TargetUnit);

	virtual void OnEquipped_Implementation(UUnitSlotComponent* SlotComponent, AUnit* TargetUnit);

	UFUNCTION(BlueprintNativeEvent, Category = "Item")
	void OnUnequipped(UUnitSlotComponent* SlotComponent, AUnit* TargetUnit);
	
	virtual void OnUnequipped_Implementation(UUnitSlotComponent* SlotComponent, AUnit* TargetUnit);

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<UUnitSlotComponent> EquippedSlot;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<AUnit> EquippedUnit;
};
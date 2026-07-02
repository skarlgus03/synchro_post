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

	// Replication
	virtual bool IsSupportedForNetworking() const override { return true; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;



	void InitializeItem(UItemDataAsset* NewItemData);


	UFUNCTION(BlueprintNativeEvent, Category = "Item")
	void OnEquipped(UUnitSlot* Slot, AUnit* TargetUnit);

	virtual void OnEquipped_Implementation(UUnitSlot* Slot, AUnit* TargetUnit);

	UFUNCTION(BlueprintNativeEvent, Category = "Item")
	void OnUnequipped();
	
	virtual void OnUnequipped_Implementation();
		

	void SetUpgradeLevel(int32 NewUpgradeLevel) { UpgradeLevel = NewUpgradeLevel; }
	int32 GetUpgradeLevel() const { return UpgradeLevel; }

	UFUNCTION(BlueprintCallable, Category = "Item")
	UItemDataAsset* GetItemDataAsset() const { return ItemData; }

	UFUNCTION(BlueprintCallable, Category = "Item")
	TArray<FStatModifier> GetItemStatModifiers() const { return ItemData ? ItemData->ItemStatModifiers : TArray<FStatModifier>(); }

	
public:


protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly , Category = "Item")
	TObjectPtr<UItemDataAsset> ItemData;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<UUnitSlot> OwnerSlot;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<AUnit> EquippedUnit;

	// Item Upgrade Level
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	int32 UpgradeLevel = 0;
	

private:


};
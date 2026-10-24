#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/SynchroPostTypes.h"
#include "ItemDataAsset.generated.h"

class UItemBase;

UCLASS()
class SYNCHROPOST_API UItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:


	// Item Display Properties
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Display")
	FText ItemName;

	// Item Description Properties
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Display", meta = (MultiLine = true))
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Display", meta = (AssetBundles = "Game"))
	TSoftObjectPtr<UTexture2D> ItemIcon;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Gameplay")
	TSubclassOf<UItemBase> ItemBaseClass;

	// Item's Stat Modifiers. This can be used to modify the player's stats when the item is equipped or used.
	// ex) Basic Sword : +10 Attack, +5 Defense , 1% LifeSteal 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemGameplay")
	TArray<FStatModifier> ItemStatModifiers;

	// Item's Equipment Tag. This can be used to categorize the item for equipment purposes.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemGameplay")
	FGameplayTag EquipmentTag;

	// Item's Rarity. This can be used to determine the item's value, drop rate, and other gameplay mechanics.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Gameplay")
	EItemRarity ItemRarity;

	// Item's Base Value. This can be used to determine the item's value in the game's economy.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Economy")
	int32 BaseValue;
};

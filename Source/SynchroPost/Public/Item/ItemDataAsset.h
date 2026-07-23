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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Display", meta = (MultiLine = true))
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Display", meta = (AssetBundles = "Game"))
	TSoftObjectPtr<UTexture2D> ItemIcon;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Gameplay")
	TSubclassOf<UItemBase> ItemBaseClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemGameplay")
	TArray<FStatModifier> ItemStatModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemGameplay")
	FGameplayTag EquipmentTag;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Gameplay")
	EItemRarity ItemRarity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Economy")
	int32 BaseValue;
};

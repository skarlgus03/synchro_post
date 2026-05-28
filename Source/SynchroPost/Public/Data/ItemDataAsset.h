
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/SynchroPostTypes.h"
#include "ItemDataAsset.generated.h"

UCLASS()
class SYNCHROPOST_API UItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TArray<FStatModifier> ItemStatModifiers;

};

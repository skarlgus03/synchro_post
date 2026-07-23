
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UnitResistanceDataAsset.generated.h"


UCLASS()
class SYNCHROPOST_API UUnitResistanceDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TMap<FGameplayTag, int32> ResistanceMap;
};

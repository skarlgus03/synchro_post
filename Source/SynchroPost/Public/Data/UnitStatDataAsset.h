#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UnitStatDataAsset.generated.h"


UCLASS()
class SYNCHROPOST_API UUnitStatDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	// Base Stats for the unit that scale with 1 level 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TMap<FGameplayTag, float> BaseStats;

	// Stat growth per level for the unit
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TMap<FGameplayTag, float> StatIncrements;
};

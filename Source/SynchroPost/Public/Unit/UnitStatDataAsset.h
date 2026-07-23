#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Unit/UnitResistanceDataAsset.h"
#include "UnitStatDataAsset.generated.h"


UCLASS()
class SYNCHROPOST_API UUnitStatDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	// Base Stats for the unit that scale with 1 level 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (Categories = "Stat.Combat"))
	TMap<FGameplayTag, int32> BaseStats;

	// Stat growth per level for the unit
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats" , meta = (Categories = "Stat.Combat"))
	TMap<FGameplayTag, int32> StatIncrements;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (Categories = "Damage"))
	TObjectPtr<UUnitResistanceDataAsset> ResistanceData;
};

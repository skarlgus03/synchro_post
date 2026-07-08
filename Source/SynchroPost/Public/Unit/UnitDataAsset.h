#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Unit/UnitStatDataAsset.h"
#include "Skill/SkillDataAsset.h"
#include "UnitDataAsset.generated.h"

UCLASS()
class SYNCHROPOST_API UUnitDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText UnitName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<USkeletalMesh> UnitMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<UUnitStatDataAsset> UnitStatData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
	TMap<FGameplayTag, TObjectPtr<USkillDataAsset>> SkillDataAssetMap;
};

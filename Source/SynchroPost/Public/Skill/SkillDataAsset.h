
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/SPSkillStructure.h"
#include "SkillDataAsset.generated.h"

class USkillBase;

UCLASS()
class SYNCHROPOST_API USkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Data")
	TArray<FSkillData> SkillDataArray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Data")
	TSubclassOf<USkillBase> SkillLogicClass;


};

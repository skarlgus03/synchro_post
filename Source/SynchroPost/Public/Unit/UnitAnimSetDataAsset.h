#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UnitAnimSetDataAsset.generated.h"

class UAnimInstance;
class UAnimMontage;

/**
 * 같은 분류의 유닛들이 공유하는 애니메이션 묶음.
 * 로코모션은 AnimBP가 처리하므로 여기엔 몽타주만 든다.
 */
UCLASS()
class SYNCHROPOST_API UUnitAnimSetDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim")
	TSoftClassPtr<UAnimInstance> AnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
	TObjectPtr<UAnimMontage> HitMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Montage")
	TObjectPtr<UAnimMontage> ReviveMontage;

};

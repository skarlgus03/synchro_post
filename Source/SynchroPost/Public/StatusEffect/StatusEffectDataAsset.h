
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/SPGameplayTags.h"
#include "Types/SPStateStructure.h"
#include "StatusEffectDataAsset.generated.h"

class UStatusEffectBase;
// 상태이상 정의 데이터 에셋
UCLASS()
class SYNCHROPOST_API UStatusEffectDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StatusEffect", meta = (Categories = "StatusEffect"))
	FGameplayTag StatusEffectTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StatusEffect|Info")
	FText DisplayName; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StatusEffect|Info")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StatusEffect|Info")
	FText Description;

	// 상태이상 적용 시 어떻게 스택을 관리할지 결정합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StatusEffect")
	EStackingPolicy StackingPolicy = EStackingPolicy::Independent;

	// 최대 스택 수. -1이면 무제한 스택입니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StatusEffect")
	int32 MaxStackCount = 1;

	UPROPERTY(EditAnywhere,BlueprintREadOnly, Instanced, Category = "StatusEffect|Logic")
	TObjectPtr<UStatusEffectBase> Logic;
};

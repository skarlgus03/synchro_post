
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Types/SPStateStructure.h"
#include "StatusEffectBase.generated.h"

class UStateComponent;
class AUnit;

UCLASS(Abstract, BlueprintType, Blueprintable)
class SYNCHROPOST_API UStatusEffectBase : public UObject
{
	GENERATED_BODY()
	

public: 

	void SetOwnerComponent(UStateComponent* StateComponent) { OwnerComp = StateComponent; }

	// 상태이상의 ID 겸 조회용 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status Effect", meta = (Categories = "StatusEffect"))
	FGameplayTag EffectTag;

	UPROPERTY(EditDefaultsOnly, Category = "Status Effect")
	EStackingPolicy StackingPolicy = EStackingPolicy::Independent;

	UPROPERTY(EditDefaultsOnly, Category = "Status Effect")
	int32 MaxStackCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Status Effect")
	TWeakObjectPtr<AUnit> SourceUnit;

	// 기본적인 4가지 훅

	UFUNCTION(BlueprintNativeEvent, Category = "Status Effect")
	void OnApply();
	virtual void OnApply_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, Category = "Status Effect")
	void OnRemove();
	virtual void OnRemove_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, Category = "Status Effect")
	void OnTurnStart();
	virtual void OnTurnStart_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, Category = "Status Effect")
	void OnTurnEnd();
	virtual void OnTurnEnd_Implementation() {}

	
	// 확장 훅 2가지

	UFUNCTION(BlueprintNativeEvent, Category = "Status Effect")
	void OnDealDamage(AUnit* Target, int32 Amount);
	virtual void OnDealDamage_Implementation(AUnit* Target, int32 Amount) {}

	UFUNCTION(BlueprintNativeEvent, Category = "Status Effect")
	void OnTakeDamage(AUnit* Source, int32 Amount);
	virtual void OnTakeDamage_Implementation(AUnit* Source, int32 Amount) {}




protected:

	UPROPERTY()
	TObjectPtr<UStateComponent> OwnerComp;
};

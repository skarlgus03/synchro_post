
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Types/SPStateStructure.h"
#include "StatusEffectBase.generated.h"

class UStateComponent;
class UStatusEffectDataAsset;

UCLASS(Abstract, EditInlineNew, BlueprintType, Blueprintable)
class SYNCHROPOST_API UStatusEffectBase : public UObject
{
	GENERATED_BODY()
	

public: 

	void SetOwnerComponent(UStateComponent* StateComponent) { OwnerComp = StateComponent; }


	// 상태이상 적용 시, 이 인스턴스를 생성한 소스를 기록한다. (장애물, 유닛 등)
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect")
	TWeakObjectPtr<AActor> Source;

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
	void OnDealDamage(AActor* Target, int32 Amount);
	virtual void OnDealDamage_Implementation(AActor* Target, int32 Amount) {}

	UFUNCTION(BlueprintNativeEvent, Category = "Status Effect")
	void OnTakeDamage(AActor* InSource, int32 Amount);
	virtual void OnTakeDamage_Implementation(AActor* InSource, int32 Amount) {}




protected:

	UPROPERTY()
	TObjectPtr<UStateComponent> OwnerComp;
};

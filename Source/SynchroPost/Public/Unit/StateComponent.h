#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Types/SPStateStructure.h"
#include "StateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStateTagRefreshed);

class UStatusEffectBase;
class AUnit;
class UTurnManager;
class UStatusEffectDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API UStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UStateComponent();
	
protected:
	
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	

	// 외부에서 조회할 때 사용
	UFUNCTION(BlueprintPure, Category = "State")
	FGameplayTagContainer GetStateTags() const;

	// 특정 태그를 가진 상태이상이 있는지 확인. (부모가 있는 태그도 포함)
	UFUNCTION(BlueprintPure, Category = "State")
	bool HasStateTag(const FGameplayTag& Tag) const;

	// UI 등에서 "이 태그를 가진 상태이상이 몇 개(중첩) 있는지" 조회
	UFUNCTION(BlueprintPure, Category = "State")
	int32 GetStatusEffectCount(const FGameplayTag& Tag) const;

	

	// 상태 추가/제거

	// 태그로 상태를 제거한다. Independent 정책은 여러 개 있을 수 있으므로 첫 번째 것만 제거한다.
	UFUNCTION(BlueprintCallable, Category = "State")
	bool RemoveFirstEffectByTag(const FGameplayTag& Tag);

	// 상태이상 추가하는 함수. 새로 걸렸거나 갱신되었으면 true를 반환함. 
	bool ApplyStatusEffect(const UStatusEffectDataAsset* Effect, int32 Duration, AActor* Source);


	

	UFUNCTION()
	void HandleUnitTurnStart(AUnit* Unit);

	UFUNCTION()
	void HandleUnitTurnEnd(AUnit* Unit);


	UPROPERTY(BlueprintAssignable, Category = "State")
	FOnStateTagRefreshed OnStateTagRefreshed;

	
protected:

	UPROPERTY(ReplicatedUsing = OnRep_StateTags)
	FStateTagList StateTagList;

	UFUNCTION()
	void OnRep_StateTags();

	// 상태이상 제거. Entry로 제거함. OnRemove를 호출함.
	void RemoveStatusEffect(FStateTagEntry Entry);

private:

	UPROPERTY()
	TObjectPtr<UTurnManager> CachedTurnManager;

	UPROPERTY()
	TObjectPtr<AUnit> OwnerUnit;


	// 턴 이벤트로 호출
	void ReduceDurationByOneTurn();
};

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
	UFUNCTION(BlueprintCallable, Category = "State")
	FGameplayTagContainer GetStateTags() const;

	UFUNCTION(BlueprintCallable, Category = "State")
	bool HasStateTag(const FGameplayTag& Tag) const;

	// UI 등에서 "이 태그를 가진 상태이상이 몇 개(중첩) 있는지" 조회
	UFUNCTION(BlueprintCallable, Category = "State")
	int32 GetStatusEffectCount(const FGameplayTag& Tag) const;

	

	// 상태 추가/제거

	// 상태를 추가한다. Duration이 -1면 영구
	UFUNCTION(BlueprintCallable, Category = "State")
	void RegisterStatusEffect(const FGameplayTag& Tag, int32 Duration, UStatusEffectBase* Instance);
	
	// 태그로 상태를 제거한다. Independent 정책은 여러 개 있을 수 있으므로 첫 번째 것만 제거한다.
	UFUNCTION(BlueprintCallable, Category = "State")
	bool RemoveFirstEffectByTag(const FGameplayTag& Tag);


	// 턴 이벤트로 호출
	UFUNCTION(BlueprintCallable, Category = "State")
	void ReduceDurationByOneTurn();
	

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


	// 상태를 제거한다. Instance는 반드시 RegisterStatusEffect에서 등록한 인스턴스여야 한다.
	void RemoveStatusEffectInstance(UStatusEffectBase* Instance);

private:

	UPROPERTY()
	TObjectPtr<UTurnManager> CachedTurnManager;

	UPROPERTY()
	TObjectPtr<AUnit> OwnerUnit;
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Types/SPStateTypes.h"
#include "StateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStateTagRefreshed);

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


	// 상태 추가/제거

	// 상태를 추가한다. Duration이 -1면 영구
	UFUNCTION(BlueprintCallable, Category = "State")
	void AddStateTag(const FGameplayTag& Tag, int32 Duration = -1);
		
	// 상태를 제거한다.
	UFUNCTION(BlueprintCallable, Category = "State")
	void RemoveStateTag(const FGameplayTag& Tag);

	// 턴 이벤트로 호출
	UFUNCTION(BlueprintCallable, Category = "State")
	void ReduceDurationByOneTurn();

	UPROPERTY(BlueprintAssignable, Category = "State")
	FOnStateTagRefreshed OnStateTagRefreshed;

	
protected:

	UPROPERTY(ReplicatedUsing = OnRep_StateTags)
	FStateTagList StateTagList;

	UFUNCTION()
	void OnRep_StateTags();

private:

};

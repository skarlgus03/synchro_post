

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurnStateComponent.generated.h"

class AUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitTurnStart, AUnit*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitTurnEnd, AUnit*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundChanged, int32, NewRound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPendingQueueChanged);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API UTurnStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UTurnStateComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	AUnit* GetCurrentUnit() const { return CurrentUnit; }
	int32 GetCurrentRound() const { return CurrentRound; }
	const TArray<TObjectPtr<AUnit>>& GetPendingQueue() const { return PendingQueue; }

	// 서버에서만 호출되어야 한다
	void SetCurrentUnit(AUnit* NewCurrentUnit);
	void SetCurrentRound(int32 NewRound);
	void SetPendingQueue(const TArray<TObjectPtr<AUnit>> NewQueue);

	UPROPERTY(BlueprintAssignable)
	FOnUnitTurnStart OnUnitTurnStart;

	UPROPERTY(BlueprintAssignable)
	FOnUnitTurnEnd OnUnitTurnEnd;

	UPROPERTY(BlueprintAssignable)
	FOnRoundChanged OnRoundChanged;

	UPROPERTY(BlueprintAssignable)
	FOnPendingQueueChanged OnPendingQueueChanged;

protected:

	UPROPERTY(ReplicatedUsing = OnRep_CurrentUnit)
	TObjectPtr<AUnit> CurrentUnit;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentRound)
	int32 CurrentRound = 0;

	UPROPERTY(ReplicatedUsing = OnRep_PendingQueue)
	TArray<TObjectPtr<AUnit>> PendingQueue;

	UFUNCTION()
	void OnRep_CurrentUnit(AUnit* OldCurrentUnit);

	UFUNCTION()
	void OnRep_CurrentRound();

	UFUNCTION()
	void OnRep_PendingQueue();
};

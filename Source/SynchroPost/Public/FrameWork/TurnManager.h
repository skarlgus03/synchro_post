
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TurnManager.generated.h"

class AUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundStart, int32, RoundNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundEnd, int32, RoundNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitTurnStart, AUnit*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitTurnEnd, AUnit*, Unit);

UCLASS()
class SYNCHROPOST_API UTurnManager : public UWorldSubsystem
{
	GENERATED_BODY()


public:

	UFUNCTION(BlueprintCallable, Category = "Turn")
	void StartCombat(const TArray<AUnit*>& InParticipants);

	UFUNCTION(BlueprintCallable, Category = "Turn")
	void EndCurrentUnitTurn();

	

	// === Getter ===

	UFUNCTION(BlueprintCallable, Category = "Turn")
	AUnit* GetCurrentUnit() const { return CurrentUnit; }

	UFUNCTION(BlueprintCallable, Category = "Turn")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintCallable, Category = "Turn")
	const TArray<AUnit*>& GetPendingQueue() const { return PendingQueue; }

public:

	UPROPERTY(BlueprintAssignable, Category = "Turn")
	FOnRoundStart OnRoundStart;

	UPROPERTY(BlueprintAssignable, Category = "Turn")
	FOnRoundEnd OnRoundEnd;

	UPROPERTY(BlueprintAssignable, Category = "Turn")
	FOnUnitTurnStart OnUnitTurnStart;

	UPROPERTY(BlueprintAssignable, Category = "Turn")
	FOnUnitTurnEnd OnUnitTurnEnd;

protected:

	UPROPERTY()
	TArray<TObjectPtr<AUnit>> Participants;

	UPROPERTY()
	TArray<TObjectPtr<AUnit>> PendingQueue;

	UPROPERTY()
	TArray<TObjectPtr<AUnit>> ActedThisRound;

	UPROPERTY()
	TObjectPtr<AUnit> CurrentUnit;

	UPROPERTY()
	int32 CurrentRound = 0;


private:

	void BeginRound();
	void EndRound();
	void AdvanceToNextUnit();
	bool IsValidParticipant(AUnit* Unit) const;
	void StartUnitTurn(AUnit* Unit);

};

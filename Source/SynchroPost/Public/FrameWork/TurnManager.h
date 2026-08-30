
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TurnManager.generated.h"

class AUnit;
class UTurnStateComponent;

UENUM(BlueprintType)
enum class ECombatResult : uint8
{
	Victory,
	Defeat
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatEnd, ECombatResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundStart, int32, RoundNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundEnd, int32, RoundNumber);

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

	AUnit* GetCurrentUnit() const;
	int32 GetCurrentRound() const;
	const TArray<AUnit*>& GetPendingQueue() const;

public:

	UPROPERTY(BlueprintAssignable, Category = "Turn")
	FOnRoundStart OnRoundStart;

	UPROPERTY(BlueprintAssignable, Category = "Turn")
	FOnRoundEnd OnRoundEnd;


	UPROPERTY(BlueprintAssignable, Category = "Turn")
	FOnCombatEnd OnCombatEnd;

protected:

	UFUNCTION()
	void HandleUnitRevived(AUnit* Unit);

	UFUNCTION()
	void HandleUnitDied(AUnit* Unit);  


protected:

	UPROPERTY()
	TArray<TObjectPtr<AUnit>> Participants;

	UPROPERTY()
	TArray<TObjectPtr<AUnit>> ActedThisRound;

private:

	void BeginRound();
	void EndRound();
	void AdvanceToNextUnit();
	bool IsValidParticipant(AUnit* Unit) const;
	void StartUnitTurn(AUnit* Unit);

	void CheckCombatEndCondition();

	UTurnStateComponent* GetTurnStateComponent() const;

	UPROPERTY()
	bool bCombatActive = false;
};

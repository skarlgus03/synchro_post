// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SPGameState.generated.h"

class UCombatEventComponent;
class UUnitSlotComponent;
class UGridStateComponent;
class UTurnStateComponent;
class URunProgressComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerListChanged);

UCLASS()
class SYNCHROPOST_API ASPGameState : public AGameState
{
	GENERATED_BODY()
	
public:

	ASPGameState();

	// 플레이어가 들어오거나 나갈 때 
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UPROPERTY(BlueprintAssignable, Category = "Player")
	FOnPlayerListChanged OnPlayerListChanged;

	UFUNCTION(BlueprintCallable, Category = "Combat Event")
	UCombatEventComponent* GetCombatEventComponent() const { return CombatEventComponent; }
		
	FORCEINLINE UUnitSlotComponent* GetUnitSlotComponent() const { return UnitSlotComponent; }
	FORCEINLINE UGridStateComponent* GetGridStateComponent() const { return GridStateComponent; }
	FORCEINLINE UTurnStateComponent* GetTurnStateComponent() const { return TurnStateComponent; }
	FORCEINLINE URunProgressComponent* GetRunProgressComponent() const { return RunProgressComponent; }


protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatEventComponent> CombatEventComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUnitSlotComponent> UnitSlotComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGridStateComponent> GridStateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTurnStateComponent> TurnStateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URunProgressComponent> RunProgressComponent;

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SPGameState.generated.h"

class UCombatEventComponent;
class UUnitSlotComponent;
class UGridStateComponent;

UCLASS()
class SYNCHROPOST_API ASPGameState : public AGameState
{
	GENERATED_BODY()
	
public:

	ASPGameState();

	UFUNCTION(BlueprintCallable, Category = "Combat Event")
	UCombatEventComponent* GetCombatEventComponent() const { return CombatEventComponent; }

	
	FORCEINLINE UUnitSlotComponent* GetUnitSlotComponent() const { return UnitSlotComponent; }
	FORCEINLINE UGridStateComponent* GetGridStateComponent() const { return GridStateComponent; }

protected:

	UPROPERTY()
	TObjectPtr<UCombatEventComponent> CombatEventComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUnitSlotComponent> UnitSlotComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UGridStateComponent> GridStateComponent;

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SPGameState.generated.h"

class UCombatEventComponent;

UCLASS()
class SYNCHROPOST_API ASPGameState : public AGameState
{
	GENERATED_BODY()
	
public:

	ASPGameState();

	UFUNCTION(BlueprintCallable, Category = "Combat Event")
	UCombatEventComponent* GetCombatEventComponent() const { return CombatEventComponent; }

protected:

	UPROPERTY()
	TObjectPtr<UCombatEventComponent> CombatEventComponent;
};

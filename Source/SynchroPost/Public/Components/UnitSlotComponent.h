// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/SynchroPostTypes.h"
#include "Components/ActorComponent.h"
#include "UnitSlotComponent.generated.h"

class AUnit;

USTRUCT(BlueprintType)
struct FUnitSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex;

	UPROPERTY(BlueprintReadOnly)
	int32 Level;

	UPROPERTY(BlueprintReadOnly)
	float Experience;

	UPROPERTY(BlueprintReadOnly)
	TArray<FStatModifier> SlotBonusStats;

};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API UUnitSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUnitSlotComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	
	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Unit Slot")
	TArray<FUnitSlot> UnitSlots;

	//UFUNCTION(BlueprintCallable, Category = "Unit Slot")
	//void ProjectUnitToSlot(int32 SlotIndex, AUnit* NewUnit);
};

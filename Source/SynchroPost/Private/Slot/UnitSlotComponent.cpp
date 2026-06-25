// Fill out your copyright notice in the Description page of Project Settings.


#include "Slot/UnitSlotComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UUnitSlotComponent::UUnitSlotComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called every frame
void UUnitSlotComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

// Called when the game starts
void UUnitSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UUnitSlotComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(UUnitSlotComponent, );
}






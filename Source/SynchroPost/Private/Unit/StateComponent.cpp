#include "Unit/StateComponent.h"

// Sets default values for this component's properties
UStateComponent::UStateComponent()
{
	
	PrimaryComponentTick.bCanEverTick = false;

}


// Called when the game starts
void UStateComponent::BeginPlay()
{
	Super::BeginPlay();

}


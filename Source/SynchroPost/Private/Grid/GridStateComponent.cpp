#include "Grid/GridStateComponent.h"
#include "Net/UnrealNetwork.h"

UGridStateComponent::UGridStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UGridStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGridStateComponent, TileGrid);
}

void UGridStateComponent::OnRep_TileGrid()
{
	OnTileGridUpdated.Broadcast();
}


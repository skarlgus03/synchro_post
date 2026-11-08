#include "Framework/SPGameState.h"
#include "Framework/CombatEventComponent.h"
#include "Slot/UnitSlotComponent.h"
#include "Grid/GridStateComponent.h"
#include "Framework/RunProgressComponent.h"
#include "Framework/TurnStateComponent.h"

ASPGameState::ASPGameState()
{
	CombatEventComponent = CreateDefaultSubobject<UCombatEventComponent>(TEXT("CombatEventComponent"));

	UnitSlotComponent = CreateDefaultSubobject<UUnitSlotComponent>(TEXT("UnitSlotComponent"));
	UnitSlotComponent->SetIsReplicated(true);

	GridStateComponent = CreateDefaultSubobject<UGridStateComponent>(TEXT("GridStateComponent"));

	TurnStateComponent = CreateDefaultSubobject<UTurnStateComponent>(TEXT("TurnStateComponent"));

	RunProgressComponent = CreateDefaultSubobject<URunProgressComponent>(TEXT("RunProgressComponent"));

	bReplicateUsingRegisteredSubObjectList = true;
}

void ASPGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	OnPlayerListChanged.Broadcast();
}

void ASPGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	OnPlayerListChanged.Broadcast();
}

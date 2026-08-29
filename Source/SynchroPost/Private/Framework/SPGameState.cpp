#include "Framework/SPGameState.h"
#include "Framework/CombatEventComponent.h"
#include "Slot/UnitSlotComponent.h"
#include "Grid/GridStateComponent.h"

ASPGameState::ASPGameState()
{
	CombatEventComponent = CreateDefaultSubobject<UCombatEventComponent>(TEXT("CombatEventComponent"));

	UnitSlotComponent = CreateDefaultSubobject<UUnitSlotComponent>(TEXT("UnitSlotComponent"));
	UnitSlotComponent->SetIsReplicated(true);

	GridStateComponent = CreateDefaultSubobject<UGridStateComponent>(TEXT("GridStateComponent"));

	bReplicateUsingRegisteredSubObjectList = true;
}

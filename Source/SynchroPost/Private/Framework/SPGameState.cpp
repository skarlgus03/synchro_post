#include "Framework/SPGameState.h"
#include "Framework/CombatEventComponent.h"
#include "Slot/UnitSlotComponent.h"

ASPGameState::ASPGameState()
{
	CombatEventComponent = CreateDefaultSubobject<UCombatEventComponent>(TEXT("CombatEventComponent"));

	UnitSlotComponent = CreateDefaultSubobject<UUnitSlotComponent>(TEXT("UnitSlotComponent"));
	UnitSlotComponent->SetIsReplicated(true);

	bReplicateUsingRegisteredSubObjectList = true;
}

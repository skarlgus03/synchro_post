#include "Framework/SPGameState.h"
#include "Framework/CombatEventComponent.h"

ASPGameState::ASPGameState()
{
	CombatEventComponent = CreateDefaultSubobject<UCombatEventComponent>(TEXT("CombatEventComponent"));
}

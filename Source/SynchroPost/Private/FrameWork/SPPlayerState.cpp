
#include "FrameWork/SPPlayerState.h"

#include "Slot/UnitSlotComponent.h"
#include "Slot/UnitSlot.h"
#include "Net/UnrealNetwork.h"

#include "Item/ItemBase.h"
#include "Item/ItemDataAsset.h"
#include "Types/SPGameplayTags.h"

ASPPlayerState::ASPPlayerState()
{

	UnitSlotComponent = CreateDefaultSubobject<UUnitSlotComponent>(TEXT("UnitSlotComponent"));
	UnitSlotComponent->SetIsReplicated(true);

	bReplicates = true;

	bReplicateUsingRegisteredSubObjectList = true;;
}

void ASPPlayerState::BeginPlay()
{

	Super::BeginPlay();

}

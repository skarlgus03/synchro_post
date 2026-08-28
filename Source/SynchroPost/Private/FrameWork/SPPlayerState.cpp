
#include "FrameWork/SPPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "Item/ItemBase.h"
#include "Item/ItemDataAsset.h"
#include "Types/SPGameplayTags.h"

ASPPlayerState::ASPPlayerState()
{

	bReplicates = true;

}

void ASPPlayerState::BeginPlay()
{

	Super::BeginPlay();

}

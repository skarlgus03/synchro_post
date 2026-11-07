
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

void ASPPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASPPlayerState, ReadyStageSerial);
}

void ASPPlayerState::SetReadyStageSerial(int32 NewSerial)
{
	if (HasAuthority())
	{
		ReadyStageSerial = NewSerial;
	}
}

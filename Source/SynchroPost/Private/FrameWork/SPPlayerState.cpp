
#include "FrameWork/SPPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "Item/ItemBase.h"
#include "Item/ItemDataAsset.h"
#include "Types/SPGameplayTags.h"
#include "Framework/RunProgressComponent.h"


ASPPlayerState::ASPPlayerState()
{

	bReplicates = true;

}

void ASPPlayerState::BeginPlay()
{

	Super::BeginPlay();

}

void ASPPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
	if (URunProgressComponent* RunProgress = URunProgressComponent::Get(this))
	{
		RunProgress->NotifyPlayerProgressChanged();
	}
}

void ASPPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASPPlayerState, ReadyStageSerial);
	DOREPLIFETIME(ASPPlayerState, RewardDoneSerial);
}

void ASPPlayerState::SetReadyStageSerial(int32 NewSerial)
{
	if (!HasAuthority() || ReadyStageSerial == NewSerial)
	{
		return;
	}
	ReadyStageSerial = NewSerial;
	OnRep_ReadyStageSerial();
}

void ASPPlayerState::SetRewardDoneSerial(int32 NewSerial)
{
	if (!HasAuthority() || RewardDoneSerial == NewSerial)
	{
		return;
	}
	RewardDoneSerial = NewSerial;
	OnRep_RewardDoneSerial();
}

void ASPPlayerState::OnRep_ReadyStageSerial()
{
	if (URunProgressComponent* RunProgress = URunProgressComponent::Get(this))
	{
		RunProgress->NotifyPlayerProgressChanged();
	}
}

void ASPPlayerState::OnRep_RewardDoneSerial()
{
	if (URunProgressComponent* RunProgress = URunProgressComponent::Get(this))
	{
		RunProgress->NotifyPlayerProgressChanged();
	}
}

#include "Grid/GridStateComponent.h"
#include "Net/UnrealNetwork.h"
#include "SynchroPost.h"

UGridStateComponent::UGridStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UGridStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGridStateComponent, TileGrid);

	DOREPLIFETIME(UGridStateComponent, GridDimensions);
	DOREPLIFETIME(UGridStateComponent, GridTileSize);
}

void UGridStateComponent::OnRep_TileGrid()
{
	OnTileGridUpdated.Broadcast();
}

void UGridStateComponent::InitializeGrid(int32 Width, int32 Height, float TileSize)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogSP, Warning, TEXT("[Grid] InitializeGrid는 서버 전용이다."));
		return;
	}

	TileGrid.InitializeGrid(Width, Height, TileSize);

	// FastArray는 Entries만 직렬화한다. 치수는 별도 복제 프로퍼티로 보내야 한다.
	GridDimensions = FIntPoint(Width, Height);
	GridTileSize = TileSize;
}


void UGridStateComponent::OnRep_GridDimensions()
{
	// 클라의 TileGrid는 Entries만 받는다. 치수를 주입해야 Find()가 동작한다.
	TileGrid.SetDimensions(GridDimensions.X, GridDimensions.Y, GridTileSize);

	TryBroadcastGridUpdated();
}

void UGridStateComponent::TryBroadcastGridUpdated()
{
	
	if (!TileGrid.IsInitialized())
	{
		return;   // 치수나 항목이 아직 덜 왔다. 이 상태로 알리면 받는 쪽이 빈 그리드를 본다
	}


	OnTileGridUpdated.Broadcast();
}
#include "Framework/SPPlayerController.h"
#include "Framework/GridVisualizer.h"
#include "Grid/TileMapDataAsset.h"
#include "Unit/Unit.h"
#include "Unit/GridMoveComponent.h"
#include "Framework/TurnManager.h"
#include "Framework/GridManager.h"

ASPPlayerController::ASPPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ASPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>())
		{
			GridManager->LoadGrid(StageData);
		}
		if (GridVisualizerClass)
		{
			GridVisualizer = GetWorld()->SpawnActor<AGridVisualizer>(GridVisualizerClass);
			GridVisualizer->PopulateFromGrid();
		}
	}

	bShowMouseCursor = true;

}

void ASPPlayerController::EnterMoveMode()
{
	UGridManager* CachedGridManager = GetWorld()->GetSubsystem<UGridManager>();
	UTurnManager* CachedTurnManager = GetWorld()->GetSubsystem<UTurnManager>();
	if (!CachedGridManager || !CachedTurnManager)
	{
		return;
	}

	MovingUnit = CachedTurnManager->GetCurrentUnit();
	if (!MovingUnit || !MovingUnit->GetGridMoveComponent())
	{
		return;
	}
	
	const FGridReachability Reachability = CachedGridManager->GetReachableTiles(
		MovingUnit->GetGridPosition(),
		MovingUnit->GetGridMoveComponent()->GetAvailableMovePoint());

	CachedReachableCoords.Empty();
	Reachability.DistanceFromStart.GetKeys(CachedReachableCoords);

	// 자기자신 제외
	TArray<FIntPoint> CoordsToHighlight = CachedReachableCoords;
	CoordsToHighlight.Remove(MovingUnit->GetGridPosition());

	GridVisualizer->AddTileStates(CoordsToHighlight, ETileVisualState::InRange);

	bIsInMoveMode = true;
}

void ASPPlayerController::ExitMoveMode()
{
	if (GridVisualizer)
	{
		if (CurrentPathCoords.Num() > 0)
		{
			GridVisualizer->RemoveTileStates(CurrentPathCoords, ETileVisualState::OnPath);
			GridVisualizer->RemoveTileState(CurrentPathCoords.Last(), ETileVisualState::Hovered);
		}
		GridVisualizer->RemoveTileStates(CachedReachableCoords, ETileVisualState::InRange);
	}

	bIsInMoveMode = false;
	MovingUnit = nullptr;
	CachedReachableCoords.Empty();
	CurrentPathCoords.Empty();
}


void ASPPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateHoverTile();
}

void ASPPlayerController::UpdateHoverTile()
{
	if (!bIsInMoveMode)
	{
		return;
	}

	float MouseX, MouseY;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	const FVector2D CurrentMousePos(MouseX, MouseY);
	if (CurrentMousePos.Equals(LastMouseScreenPosition, 0.5f))
	{
		return;
	}
	LastMouseScreenPosition = CurrentMousePos;

	if (!GridVisualizer || !MovingUnit)
	{
		return;
	}

	UGridManager* CachedGridManager = GetWorld()->GetSubsystem<UGridManager>();
	if (!CachedGridManager)
	{
		return;
	}

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		return;
	}

	const FIntPoint HoveredCoord = CachedGridManager->WorldLocationToCoord(Hit.Location);
	if (HoveredCoord == LastHoveredCoord)
	{
		return;
	}
	LastHoveredCoord = HoveredCoord;

	// 이전 경로 상태 정리 (OnPath 제거, 마지막 타일의 Hovered도 제거)
	if (CurrentPathCoords.Num() > 0)
	{
		GridVisualizer->RemoveTileStates(CurrentPathCoords, ETileVisualState::OnPath);
		GridVisualizer->RemoveTileState(CurrentPathCoords.Last(), ETileVisualState::Hovered);
		CurrentPathCoords.Empty();
	}

	if (!CachedReachableCoords.Contains(HoveredCoord))
	{
		return; // 범위 밖이면 여기서 끝
	}

	const TArray<FIntPoint> Path = CachedGridManager->FindPath(
		MovingUnit->GetGridPosition(),
		HoveredCoord,
		MovingUnit->GetGridMoveComponent()->GetAvailableMovePoint());

	if (Path.Num() == 0)
	{
		return;
	}

	CurrentPathCoords = Path;
	GridVisualizer->AddTileStates(Path, ETileVisualState::OnPath);
	GridVisualizer->AddTileState(Path.Last(), ETileVisualState::Hovered);
}


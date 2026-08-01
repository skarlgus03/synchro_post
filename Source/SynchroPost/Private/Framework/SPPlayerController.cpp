#include "Framework/SPPlayerController.h"
#include "Framework/GridVisualizer.h"
#include "Grid/TileMapDataAsset.h"
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

void ASPPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!GridVisualizer)
	{
		return;
	}

	UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();

	if (!GridManager)
	{
		return;
	}

	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		const FIntPoint Coord = GridManager->WorldLocationToCoord(Hit.Location);
		UE_LOG(LogTemp, Log, TEXT("Hovered Coord: (%d, %d)"), Coord.X, Coord.Y);
		if (Coord != LastHoveredCoord)
		{
			LastHoveredCoord = Coord;
			GridVisualizer->ShowHighlightedTiles({ Coord });
		}
	}
}

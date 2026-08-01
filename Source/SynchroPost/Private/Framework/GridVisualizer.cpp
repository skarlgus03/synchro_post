#include "Framework/GridVisualizer.h"
#include "Framework/GridManager.h"
#include "Components/InstancedStaticMeshComponent.h"


AGridVisualizer::AGridVisualizer()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseGridMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BaseGridMesh"));
	RootComponent = BaseGridMesh;

	HighlightMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HighlightMesh"));
	HighlightMesh->SetupAttachment(RootComponent);
}

void AGridVisualizer::PopulateFromGrid()
{
	if (!CachedGridManager)
	{
		CachedGridManager = GetWorld()->GetSubsystem<UGridManager>();
	}
	if (!CachedGridManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridVisualizer: No GridManager found."));
		return;
	}

	BaseGridMesh->ClearInstances();

	for (const FTile& Tile : CachedGridManager->GetAllTiles())
	{
		const FVector InstanceLocation = Tile.WorldLocation + FVector(0.f, 0.f, BaseHeightOffset);
		const FTransform InstanceTransform(FRotator::ZeroRotator, InstanceLocation, FVector(TileScale, TileScale, 1.0f));
		BaseGridMesh->AddInstance(InstanceTransform, true);
	}
}

void AGridVisualizer::ClearAll()
{
	BaseGridMesh->ClearInstances();
	HighlightMesh->ClearInstances();
}

void AGridVisualizer::ShowHighlightedTiles(const TArray<FIntPoint>& Coords)
{
	if (!CachedGridManager)
	{
		CachedGridManager = GetWorld()->GetSubsystem<UGridManager>();
	}
	if (!CachedGridManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridVisualizer: No GridManager found."));
		return;
	}

	HighlightMesh->ClearInstances();
	
	for (const FIntPoint& Coord : Coords)
	{
		const FVector WorldLoc = CachedGridManager->GetTileWorldLocation(Coord) + FVector(0.f,0.f,HighlightHeightOffset);
		const FTransform InstanceTransform(FRotator::ZeroRotator, WorldLoc, FVector(TileScale, TileScale, 1.0f));
		HighlightMesh->AddInstance(InstanceTransform, true);
	}
	UE_LOG(LogTemp, Log, TEXT("GridVisualizer: Highlighted %d tiles."), Coords.Num());
}

void AGridVisualizer::ClearHighlightedTiles()
{
	HighlightMesh->ClearInstances();
}




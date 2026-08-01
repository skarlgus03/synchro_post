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
		const FTransform InstanceTransform(FRotator::ZeroRotator, Tile.WorldLocation, FVector::OneVector);
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
		const FVector WorldLoc = CachedGridManager->GetTileWorldLocation(Coord);
		const FTransform InstanceTransform(FRotator::ZeroRotator, WorldLoc, FVector::OneVector);
		HighlightMesh->AddInstance(InstanceTransform, true);
	}
}

void AGridVisualizer::ClearHighlightedTiles()
{
	HighlightMesh->ClearInstances();
}




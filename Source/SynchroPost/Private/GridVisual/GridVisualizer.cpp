#include "GridVisual/GridVisualizer.h"
#include "Framework/GridManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "SynchroPost.h"
#include "Engine/StaticMesh.h"
#include "GridVisual/GridOutlineComponent.h"
#include "Math/GridPath.h"
#include "GridVisual/GridPathComponent.h"


AGridVisualizer::AGridVisualizer()
{
	PrimaryActorTick.bCanEverTick = false;

	GridMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GridMesh"));
	RootComponent = GridMesh;

	Outline = CreateDefaultSubobject<UGridOutlineComponent>(TEXT("Outline"));
	Outline->SetupAttachment(RootComponent);

	PathArrow = CreateDefaultSubobject<UGridPathComponent>(TEXT("PathArrow"));
	PathArrow->SetupAttachment(RootComponent);
}


void AGridVisualizer::PopulateFromGrid()
{
	if (!CachedGridManager)
	{
		CachedGridManager = GetWorld()->GetSubsystem<UGridManager>();
	}
	if (!CachedGridManager)
	{
		return;
	}
	GridMesh->ClearInstances();

	const float TileSize = CachedGridManager->GetTileSize();
	const UStaticMesh* Mesh = GridMesh->GetStaticMesh();
	const FVector MeshSize = Mesh ? Mesh->GetBounds().BoxExtent * 2.0f : FVector::ZeroVector;
	if (TileSize <= 0.f || MeshSize.IsNearlyZero())
	{
		UE_LOG(LogSP, Error, TEXT("[GridVisualizer] 스케일 계산 불가 - 타일 크기 또는 메시 크기 확인 필요"));
		return;
	}

	const FVector InstanceScale(
		TileSize * TileScale / MeshSize.X,
		TileSize * TileScale / MeshSize.Y,
		1.0f
	);

	for (const FTile& Tile : CachedGridManager->GetAllTiles())
	{
		const FVector InstanceLocation = Tile.WorldLocation + FVector(0.f, 0.f, HeightOffset);
		GridMesh->AddInstance(FTransform(FRotator::ZeroRotator, InstanceLocation, InstanceScale), /*bWorldSpace*/ true);
	}
}

void AGridVisualizer::ApplyPreview(const FGridPreview& Preview)
{
	if (Preview.RangeTiles != CurrentPreview.RangeTiles)
	{
		Outline->SetLayer(EGridOutlineLayer::Range, Preview.RangeTiles);
	}
	Outline->SetLayer(EGridOutlineLayer::Impact, Preview.ImpactTiles);
	Outline->SetLayer(EGridOutlineLayer::Target,
		Preview.TargetTile.IsSet() ? TArray<FIntPoint>{ Preview.TargetTile.GetValue() } : TArray<FIntPoint>());

	// 경로 화살표: 유닛 이동과 같은 함수로 부드럽게 (높이는 컴포넌트가 띄운다)
	if (Preview.PathTiles != CurrentPreview.PathTiles)
	{
		const UGridManager* Grid = GetWorld() ? GetWorld()->GetSubsystem<UGridManager>() : nullptr;
		if (Grid && Preview.PathTiles.Num() >= 2)
		{
			PathArrow->SetPath(GridPath::BuildSmoothPath(*Grid, Preview.PathTiles,
				[Grid](const FIntPoint& Coord) { return Grid->GetTileWorldLocation(Coord); }));
		}
		else
		{
			PathArrow->ClearPath();
		}
	}

	CurrentPreview = Preview;
}

void AGridVisualizer::ClearPreview()
{
	Outline->ClearAllLayers();
	PathArrow->ClearPath();
	CurrentPreview = FGridPreview();
}








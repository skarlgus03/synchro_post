#include "GridVisual/GridVisualizer.h"
#include "Framework/GridManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "SynchroPost.h"
#include "Engine/StaticMesh.h"
#include "GridVisual/GridOutlineComponent.h"
#include "Math/GridPath.h"
#include "GridVisual/GridPathComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "GridVisual/GridTileOverlayComponent.h"
#include "Types/SPGameplayTags.h"
#include "GridVisual/GridGhostComponent.h"

AGridVisualizer::AGridVisualizer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	GridMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GridMesh"));
	RootComponent = GridMesh;

	Outline = CreateDefaultSubobject<UGridOutlineComponent>(TEXT("Outline"));
	Outline->SetupAttachment(RootComponent);

	PathArrow = CreateDefaultSubobject<UGridPathComponent>(TEXT("PathArrow"));
	PathArrow->SetupAttachment(RootComponent);

	TileOverlay = CreateDefaultSubobject<UGridTileOverlayComponent>(TEXT("TileOverlay"));
	TileOverlay->SetupAttachment(RootComponent);

	GhostMesh = CreateDefaultSubobject<UGridGhostComponent>(TEXT("GhostMesh"));
	GhostMesh->SetupAttachment(RootComponent);
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

	// 장애물 칸은 판이 아님
	const TSet<FIntPoint> Obstacles(CachedGridManager->GetObstacleTiles());


	for (const FTile& Tile : CachedGridManager->GetAllTiles())
	{
		if (Obstacles.Contains(Tile.Coordinate))
		{
			continue;
		}
		const FVector InstanceLocation = Tile.WorldLocation + FVector(0.f, 0.f, HeightOffset);
		GridMesh->AddInstance(FTransform(FRotator::ZeroRotator, InstanceLocation, InstanceScale), /*bWorldSpace*/ true);
	}
}

void AGridVisualizer::ApplyPreview(const FGridPreview& Preview)
{
	if (Preview.RangeTiles != CurrentPreview.RangeTiles)
	{
		Outline->SetLayer(EGridOutlineLayer::Range, Preview.RangeTiles);
		TileOverlay->SetLayer(SPTags::Grid::Overlay::Range, Preview.RangeTiles);
	}
	Outline->SetLayer(EGridOutlineLayer::Impact, Preview.ImpactTiles);
	TileOverlay->SetLayer(SPTags::Grid::Overlay::Impact, Preview.ImpactTiles);
	
	TileOverlay->SetLayer(SPTags::Grid::Overlay::Target,
		Preview.TargetTile.IsSet() ? TArray<FIntPoint>{ Preview.TargetTile.GetValue() } : TArray<FIntPoint>());
	// 경로 화살표: 유닛 이동과 같은 함수로 부드럽게 (높이는 컴포넌트가 띄운다)
	if (Preview.PathTiles != CurrentPreview.PathTiles)
	{
		const UGridManager* Grid = GetWorld() ? GetWorld()->GetSubsystem<UGridManager>() : nullptr;
		if (Grid && Preview.PathTiles.Num() >= 2)
		{
			PathArrow->SetPath(GridPath::BuildTilePath(*Grid, Preview.PathTiles,
				[Grid](const FIntPoint& Coord) { return Grid->GetTileWorldLocation(Coord); }));
		}
		else
		{
			PathArrow->ClearPath();
		}
	}

	if (Preview.Ghost.IsSet() && Preview.Ghost->Unit.IsValid())
	{
		GhostMesh->ShowGhost(Preview.Ghost->Unit.Get(), Preview.Ghost->Coord, Preview.Ghost->FacingFrom);
	}
	else
	{
		GhostMesh->HideGhost();
	}


	CurrentPreview = Preview;
}

void AGridVisualizer::ClearPreview()
{
	CurrentPreview = FGridPreview();
	
	// 격자에 보이는 중이면 미리보기 메시를 지우지 않고, 격자가 완전히 숨겨지면 지운다
	if (CurrentReveal > 0.f)
	{
		bClearWhenHidden = true;
	}
	else
	{
		ClearPreviewMeshes();
	}
}

void AGridVisualizer::SetGridRevealed(bool bRevealed)
{
	TargetReveal = bRevealed ? 1.f : 0.f;

	if (GetFadeDuration() <= 0.f)
	{
		CurrentReveal = TargetReveal;
		ApplyReveal(CurrentReveal);
		return;
	}
	SetActorTickEnabled(CurrentReveal != TargetReveal);
}

void AGridVisualizer::ApplyReveal(float Value)
{
	if (RevealCollection)
	{
		if (UMaterialParameterCollectionInstance* Instance = GetWorld()->GetParameterCollectionInstance(RevealCollection))
		{
			Instance->SetScalarParameterValue(TEXT("GridReveal"), Value);
		}
	}

	// 완전히 숨으면 그리지도 않는다 (자식인 Outline/PathArrow에는 전파 안 함)
	GridMesh->SetVisibility(Value > 0.f);

	if (Value <= 0.f && bClearWhenHidden)
	{
		ClearPreviewMeshes();
	}
}

void AGridVisualizer::ClearPreviewMeshes()
{
	bClearWhenHidden = false;
	Outline->ClearAllLayers();
	TileOverlay->ClearAllLayers();
	PathArrow->ClearPath();
	GhostMesh->HideGhost();
}

void AGridVisualizer::BeginPlay()
{
	Super::BeginPlay();

	ApplyReveal(0.f);	// 평소엔 숨긴다
}

void AGridVisualizer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentReveal = FMath::FInterpConstantTo(CurrentReveal, TargetReveal, DeltaTime, 1.f / GetFadeDuration());
	ApplyReveal(CurrentReveal);
	if (CurrentReveal == TargetReveal)
	{
		SetActorTickEnabled(false);
	}
}









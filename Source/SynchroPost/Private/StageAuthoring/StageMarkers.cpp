#include "StageAuthoring/StageMarkers.h"

#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Grid/ObstacleDataAsset.h"
#include "StageAuthoring/StageBaker.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/BoxComponent.h"

AStageMarker::AStageMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetActorHiddenInGame(true);

#if WITH_EDITORONLY_DATA
	bIsEditorOnlyActor = true;
#endif

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(Root);
	// 베이커 레이저에 안 걸리게. 콜리전이 없어도 뷰포트 클릭 선택은 된다(렌더링 기준)
	PreviewMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	PreviewMesh->SetCanEverAffectNavigation(false);
	PreviewMesh->SetHiddenInGame(true);

#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(Root);
		Arrow->ArrowSize = 2.0f;
	}
#endif

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	FallbackMesh = ConeMesh.Object;
	SetPreviewMesh(nullptr);
}

void AStageMarker::SetPreviewMesh(UStaticMesh* Mesh)
{
	PreviewMesh->SetStaticMesh(Mesh ? Mesh : FallbackMesh.Get());
}

void AStagePropMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetPreviewMesh(Mesh.LoadSynchronous());
}

void AStageEntityMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetPreviewMesh(nullptr);   // BP 액터 모양은 미리 그리기 어려워 기본 원뿔
}

void AStageExitMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetPreviewMesh(Mesh.LoadSynchronous());
}

void AGridObstacleMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetPreviewMesh(ObstacleData ? ObstacleData->ObstacleMesh.LoadSynchronous() : nullptr);

	PreviewMesh->SetRelativeTransform(ObstacleData ? ObstacleData->MeshRelativeTransform : FTransform::Identity);
}

#if WITH_EDITOR
void AGridObstacleMarker::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (!bFinished)
	{
		return;   // 드래그 중엔 자유롭게, 놓을 때만 스냅
	}

	const AStageBaker* Baker = AStageBaker::FindInLevel(GetLevel());
	FIntPoint Coord;
	if (Baker && Baker->WorldToCoord(GetActorLocation(), Coord))
	{
		SetActorLocation(Baker->GetTileCenter(Coord));
	}
}
#endif

// ---------- 스폰 볼륨 ----------

AGridSpawnVolume::AGridSpawnVolume()
{
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(GetRootComponent());
	Box->SetBoxExtent(FVector(90.0f, 90.0f, 50.0f));   // 기본: 180cm 칸 하나
	Box->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);   // 베이커 레이저에 안 걸리게
	Box->SetCanEverAffectNavigation(false);
	Box->SetHiddenInGame(true);
	Box->SetLineThickness(4.0f);
	PreviewMesh->SetStaticMesh(nullptr);
#if WITH_EDITORONLY_DATA
	if (Arrow)
	{
		Arrow->SetVisibility(false);
	}
#endif
}

void AGridSpawnVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Box->ShapeColor = (Team == ESpawnVolumeTeam::Ally) ? FColor(80, 150, 255) : FColor(255, 150, 60);
	Box->MarkRenderStateDirty();

	// 끄는 중 / 크기·팀을 바꿀 때마다 불린다 → 미리보기가 따라온다
	RefreshBakerPreview();
}

bool AGridSpawnVolume::ContainsXY(const FVector& WorldLocation) const
{
	const FVector Local = Box->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector Extent = Box->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) < Extent.X && FMath::Abs(Local.Y) < Extent.Y;
}

void AGridSpawnVolume::FitToSingleTile(float TileSize)
{
	SetActorScale3D(FVector::OneVector);
	Box->SetBoxExtent(FVector(TileSize * 0.5f, TileSize * 0.5f, 50.0f));
}

void AGridSpawnVolume::RefreshBakerPreview() const
{
#if WITH_EDITOR
	if (AStageBaker* Baker = AStageBaker::FindInLevel(GetLevel()))
	{
		Baker->RefreshPreview();
	}
#endif
}
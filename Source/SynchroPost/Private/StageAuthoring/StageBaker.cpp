#include "StageAuthoring/StageBaker.h"
#include "Components/BillboardComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFlow/StageDataAsset.h"
#include "Grid/TileMapDataAsset.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "SynchroPost.h"
#include "DrawDebugHelpers.h"

AStageBaker::AStageBaker()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

#if WITH_EDITORONLY_DATA
	bIsEditorOnlyActor = true;
#endif

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

#if WITH_EDITORONLY_DATA
	// 비전투 DA일 땐 미리보기가 없으니 선택용 아이콘
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(Root);
	}
#endif

	PreviewMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PreviewMesh"));
	PreviewMesh->SetupAttachment(Root);
	PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // 자기 트레이스에 안 걸리게
	PreviewMesh->SetCanEverAffectNavigation(false);
	PreviewMesh->SetCastShadow(false);
	PreviewMesh->SetHiddenInGame(true);
	PreviewMesh->SetUsingAbsoluteRotation(true);                       // 베이커를 돌려도 그리드는 축 정렬
	PreviewMesh->SetUsingAbsoluteScale(true);
	PreviewMesh->NumCustomDataFloats = 4;                              // RGBA (GridVisualizer와 같은 머티리얼 규약)

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		PreviewMesh->SetStaticMesh(PlaneMesh.Object);
	}
}

void AStageBaker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	bTargetIsCombat = Target && Target->IsA<UCombatStageDataAsset>();
	RebuildPreview();
}

void AStageBaker::RefreshPreview()
{
	RebuildPreview();
}

FVector AStageBaker::CoordToWorld(const FIntPoint& Coord) const
{
	return GetActorLocation() + FVector(Coord.X * TileSize, Coord.Y * TileSize, 0.0f);
}

bool AStageBaker::ProbeTile(const FIntPoint& Coord, FVector& OutSurface) const
{
	const FVector Center = CoordToWorld(Coord);
	OutSurface = Center;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Start = Center + FVector(0.0f, 0.0f, ClearanceHeight);
	const FVector End = Center - FVector(0.0f, 0.0f, TraceDepth);

	const FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic);
	// 판정은 전부 단순 콜리전 기준. 오버랩은 속이 찬 볼륨이 필요하고, 바닥은 평평해야 하므로 정밀한 표면이 필요 없다
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(StageBakerProbe), /*bTraceComplex*/ false, this);

	// 1) 서 있을 높이가 지형 안 → 키 큰 벽
	if (World->OverlapAnyTestByObjectType(Start, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(1.0f), Params))
	{
#if ENABLE_DRAW_DEBUG
		if (bDrawProbeDebug)
		{
			DrawDebugSphere(World, Start, 12.0f, 8, FColor::Magenta, /*bPersistent*/ true);
		}
#endif
		OutSurface = Start;
		return false;
	}

	// 2) 아래로 쏴서 바닥 찾기
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, Params);

	// 3) 원점 높이에서 벗어나면 단차/꺼진 바닥
	const bool bFloor = bHit && FMath::Abs(Hit.ImpactPoint.Z - Center.Z) <= FloorTolerance;

#if ENABLE_DRAW_DEBUG
	if (bDrawProbeDebug)
	{
		if (bHit)
		{
			const FColor Color = bFloor ? FColor::Green : FColor::Yellow;
			DrawDebugLine(World, Start, Hit.ImpactPoint, Color, /*bPersistent*/ true, -1.0f, 0, 2.0f);
			DrawDebugPoint(World, Hit.ImpactPoint, 10.0f, Color, /*bPersistent*/ true);
		}
		else
		{
			DrawDebugLine(World, Start, End, FColor::Blue, /*bPersistent*/ true, -1.0f, 0, 2.0f);
		}
	}
#endif

	if (!bHit)
	{
		return false;
	}
	OutSurface = Hit.ImpactPoint;
	return bFloor;
}

void AStageBaker::RebuildPreview()
{
	PreviewMesh->ClearInstances();

	UWorld* World = GetWorld();

#if ENABLE_DRAW_DEBUG
	if (World && !World->IsGameWorld())
	{
		FlushPersistentDebugLines(World);   // 이전 레이저 지우기
	}
#endif

	if (!bTargetIsCombat || !World || World->IsGameWorld())
	{
		return;
	}

	UStaticMesh* Mesh = PreviewMesh->GetStaticMesh();
	if (!Mesh || TileSize <= 0.0f)
	{
		return;
	}

	PreviewMesh->SetMaterial(0, PreviewMaterial.LoadSynchronous());

	// 메시가 100cm라고 가정하지 않는다 — 바운즈에서 원본 크기
	const FVector MeshSize = Mesh->GetBounds().BoxExtent * 2.0;
	if (MeshSize.X <= KINDA_SMALL_NUMBER || MeshSize.Y <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Scale(
		TileSize * PreviewTileScale / MeshSize.X,
		TileSize * PreviewTileScale / MeshSize.Y,
		1.0f);

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			FVector Surface;
			const bool bFloor = ProbeTile(FIntPoint(X, Y), Surface);

			const FTransform InstanceTransform(FRotator::ZeroRotator, Surface + FVector(0.0f, 0.0f, PreviewHeightOffset), Scale);
			const int32 Index = PreviewMesh->AddInstance(InstanceTransform, /*bWorldSpace*/ true);

			const FLinearColor& Color = bFloor ? FloorColor : WallColor;
			PreviewMesh->SetCustomDataValue(Index, 0, Color.R);
			PreviewMesh->SetCustomDataValue(Index, 1, Color.G);
			PreviewMesh->SetCustomDataValue(Index, 2, Color.B);
			PreviewMesh->SetCustomDataValue(Index, 3, Color.A);
		}
	}

	PreviewMesh->MarkRenderStateDirty();
}

void AStageBaker::Bake()
{
#if WITH_EDITOR
	if (!Target)
	{
		UE_LOG(LogSP, Error, TEXT("[StageBaker] Target이 비어 있다."));
		return;
	}

	// 서브레벨에 배치해도 맞게: 이 액터가 속한 레벨의 월드 에셋
	UWorld* LevelWorld = GetLevel() ? GetLevel()->GetTypedOuter<UWorld>() : nullptr;
	if (!LevelWorld)
	{
		UE_LOG(LogSP, Error, TEXT("[StageBaker] 소속 레벨을 찾지 못했다."));
		return;
	}

	UTileMapDataAsset* TileMap = nullptr;
	if (const UCombatStageDataAsset* CombatStage = Cast<UCombatStageDataAsset>(Target))
	{
		TileMap = CombatStage->TileMap;
		if (!TileMap)
		{
			UE_LOG(LogSP, Error, TEXT("[StageBaker] %s: 전투 DA의 TileMap이 비어 있다. 굽기 중단."), *Target->GetName());
			return;
		}
	}

	// 검증이 다 끝난 뒤에 쓰기 시작한다 (반쯤 구운 상태를 만들지 않음)
	Target->Modify();
	Target->Level = LevelWorld;
	Target->MarkPackageDirty();

	if (TileMap)
	{
		TileMap->Modify();
		TileMap->GridWidth = GridWidth;
		TileMap->GridHeight = GridHeight;
		TileMap->TileSize = TileSize;
		TileMap->GridOrigin = GetActorLocation();

		TileMap->WallTiles.Reset();
		for (int32 Y = 0; Y < GridHeight; ++Y)
		{
			for (int32 X = 0; X < GridWidth; ++X)
			{
				FVector Surface;
				if (!ProbeTile(FIntPoint(X, Y), Surface))
				{
					TileMap->WallTiles.Add(FIntPoint(X, Y));
				}
			}
		}
		TileMap->MarkPackageDirty();

		UE_LOG(LogSP, Log, TEXT("[StageBaker] %s → %s: %dx%d, 타일 %.0f, 원점 %s, 벽 %d칸"),
			*Target->GetName(), *TileMap->GetName(), GridWidth, GridHeight, TileSize,
			*GetActorLocation().ToCompactString(), TileMap->WallTiles.Num());
	}
	else
	{
		UE_LOG(LogSP, Log, TEXT("[StageBaker] %s: Level = %s"), *Target->GetName(), *LevelWorld->GetName());
	}

	RebuildPreview();
#endif
}// Fill out your copyright notice in the Description page of Project Settings.




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
#include "StageAuthoring/StageMarkers.h"
#include "Misc/MessageDialog.h"

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

#if WITH_EDITOR
	const TMap<FIntPoint, ETileType> SpawnTiles = CollectSpawnTiles(/*bLogWarnings*/ false);
#else
	const TMap<FIntPoint, ETileType> SpawnTiles;
#endif

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const FIntPoint Coord(X, Y);
			FVector Surface;
			const bool bFloor = ProbeTile(Coord, Surface);

			const FTransform InstanceTransform(FRotator::ZeroRotator, Surface + FVector(0.0f, 0.0f, PreviewHeightOffset), Scale);
			const int32 Index = PreviewMesh->AddInstance(InstanceTransform, /*bWorldSpace*/ true);

			// 벽이 스폰보다 우선 (Bake도 벽 위 스폰은 버림)
			FLinearColor Color = bFloor ? FloorColor : WallColor;
			if (bFloor)
			{
				if (const ETileType* Type = SpawnTiles.Find(Coord))
				{
					Color = (*Type == ETileType::SpawnPoint) ? AllySpawnColor : EnemySpawnColor;
				}
			}

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

	UWorld* LevelWorld = GetOwningLevelWorld();
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

	if (LoadedStage && LoadedStage != Target)
	{
		UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s에서 불러온 마커를 %s로 굽는다."), *LoadedStage->GetName(), *Target->GetName());
	}

	// 검증이 다 끝난 뒤에 쓰기 시작한다 (반쯤 구운 상태를 만들지 않음)
	Target->Modify();
	Target->Level = LevelWorld;
	BakeSlots(Target);
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

		BakeSpawns(TileMap);      // 벽 계산 뒤 (벽 칸 제외)
		BakeObstacles(TileMap);   // 스폰 뒤 (스폰 칸 위 장애물 경고에 씀)
		TileMap->MarkPackageDirty();
	}
	else
	{
		const int32 IgnoredVolumes = GatherMarkers<AGridSpawnVolume>().Num();
		if (IgnoredVolumes > 0)
		{
			UE_LOG(LogSP, Warning, TEXT("[StageBaker] 비전투 DA라 스폰 볼륨 %d개는 무시됨."), IgnoredVolumes);
		}
	}

	UE_LOG(LogSP, Log, TEXT("[StageBaker] %s 굽기 완료: 프롭 %d / 엔티티 %d / 출구 %d%s"),
		*Target->GetName(), Target->PropSlots.Num(), Target->EntitySlots.Num(), Target->ExitSlots.Num(),
		TileMap ? *FString::Printf(TEXT(" / %dx%d, 벽 %d, 스폰 %d, 장애물 %d"), GridWidth, GridHeight,
			TileMap->WallTiles.Num(), TileMap->SpecialTiles.Num(), TileMap->Obstacles.Num()) : TEXT(""));

	RebuildPreview();
#endif
}



bool AStageBaker::WorldToCoord(const FVector& WorldLocation, FIntPoint& OutCoord) const
{
	if (TileSize <= 0.0f)
	{
		return false;
	}

	const FVector Local = WorldLocation - GetActorLocation();
	OutCoord = FIntPoint(FMath::RoundToInt(Local.X / TileSize), FMath::RoundToInt(Local.Y / TileSize));
	return OutCoord.X >= 0 && OutCoord.X < GridWidth && OutCoord.Y >= 0 && OutCoord.Y < GridHeight;
}

AStageBaker* AStageBaker::FindInLevel(const ULevel* Level)
{
	if (!Level)
	{
		return nullptr;
	}
	for (AActor* Actor : Level->Actors)
	{
		if (AStageBaker* Baker = Cast<AStageBaker>(Actor))
		{
			return Baker;
		}
	}
	return nullptr;
}

void AStageBaker::LoadFromTarget()
{
#if WITH_EDITOR
	if (!Target)
	{
		UE_LOG(LogSP, Error, TEXT("[StageBaker] Target이 비어 있다."));
		return;
	}

	if (!ConfirmDiscardMarkers(FString::Printf(TEXT("%s를 불러올까?"), *Target->GetName())))
	{
		return;
	}

	if (!Target->Level.IsNull() && Target->Level.ToSoftObjectPath() != FSoftObjectPath(GetOwningLevelWorld()))
	{
		UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s는 다른 레벨(%s)용이다. 불러오긴 하지만 Bake하면 이 레벨로 바뀜."),
			*Target->GetName(), *Target->Level.ToString());
	}

	DestroyAllMarkers();
	LoadSlots(Target);

	if (const UCombatStageDataAsset* CombatStage = Cast<UCombatStageDataAsset>(Target))
	{
		if (CombatStage->TileMap)
		{
			LoadSpawns(CombatStage->TileMap);
			LoadObstacles(CombatStage->TileMap);
		}
	}

	Modify();
	LoadedStage = Target;
	RebuildPreview();   

	UE_LOG(LogSP, Log, TEXT("[StageBaker] %s 불러옴: 마커 %d개"), *Target->GetName(), GatherMarkers<AStageMarker>().Num());
#endif
}

void AStageBaker::ClearMarkers()
{
#if WITH_EDITOR
	if (!ConfirmDiscardMarkers(TEXT("전부 지울까?")))
	{
		return;
	}

	const int32 Count = DestroyAllMarkers();
	Modify();
	LoadedStage = nullptr;
	RebuildPreview();
	UE_LOG(LogSP, Log, TEXT("[StageBaker] 마커 %d개 제거"), Count);
#endif
}

UWorld* AStageBaker::GetOwningLevelWorld() const
{
	// 서브레벨에 배치해도 맞게: 이 액터가 속한 레벨의 월드 에셋
	return GetLevel() ? GetLevel()->GetTypedOuter<UWorld>() : nullptr;
}

bool AStageBaker::ConfirmDiscardMarkers(const FString& Action) const
{
	const int32 Count = GatherMarkers<AStageMarker>().Num();
	if (Count == 0)
	{
		return true;
	}

	const FText Message = FText::FromString(FString::Printf(
		TEXT("레벨에 마커 %d개가 있다. 지우고 %s\n(굽지 않은 변경은 사라진다)"), Count, *Action));
	return FMessageDialog::Open(EAppMsgType::YesNo, Message) == EAppReturnType::Yes;
}

int32 AStageBaker::DestroyAllMarkers()
{
	TArray<AStageMarker*> Markers = GatherMarkers<AStageMarker>();
	for (AStageMarker* Marker : Markers)
	{
		Marker->Destroy();
	}
	return Markers.Num();
}

void AStageBaker::BakeSlots(UStageDataAsset* Stage) const
{
	Stage->PropSlots.Reset();
	for (const AStagePropMarker* Marker : GatherMarkers<AStagePropMarker>())
	{
		if (Marker->Mesh.IsNull())
		{
			UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s: Mesh가 비어 있어 건너뜀"), *Marker->GetActorLabel());
			continue;
		}
		FPropSlotInfo& Slot = Stage->PropSlots.AddDefaulted_GetRef();
		Slot.SlotType = Marker->SlotType;
		Slot.Transform = Marker->GetActorTransform();
		Slot.Mesh = Marker->Mesh;
	}

	Stage->EntitySlots.Reset();
	for (const AStageEntityMarker* Marker : GatherMarkers<AStageEntityMarker>())
	{
		if (Marker->ActorClass.IsNull())
		{
			UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s: ActorClass가 비어 있어 건너뜀"), *Marker->GetActorLabel());
			continue;
		}
		FEntitySlotInfo& Slot = Stage->EntitySlots.AddDefaulted_GetRef();
		Slot.SlotType = Marker->SlotType;
		Slot.ActorClass = Marker->ActorClass;
		Slot.Transform = Marker->GetActorTransform();
	}

	Stage->ExitSlots.Reset();
	for (const AStageExitMarker* Marker : GatherMarkers<AStageExitMarker>())
	{
		if (Marker->Mesh.IsNull())
		{
			UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s: Mesh가 비어 있어 건너뜀"), *Marker->GetActorLabel());
			continue;
		}
		FExitSlotInfo& Slot = Stage->ExitSlots.AddDefaulted_GetRef();
		Slot.Position = Marker->Position;
		Slot.Transform = Marker->GetActorTransform();
		Slot.Mesh = Marker->Mesh;
	}
}

void AStageBaker::BakeObstacles(UTileMapDataAsset* TileMap) const
{
	const TSet<FIntPoint> Walls(TileMap->WallTiles);

	TSet<FIntPoint> SpawnTiles;
	for (const FTileSpawnInfo& Special : TileMap->SpecialTiles)
	{
		if (Special.TileType != ETileType::Normal)
		{
			SpawnTiles.Add(Special.Coordinate);
		}
	}

	TSet<FIntPoint> Used;
	TileMap->Obstacles.Reset();

	for (const AGridObstacleMarker* Marker : GatherMarkers<AGridObstacleMarker>())
	{
		const FString Label = Marker->GetActorLabel();

		if (!Marker->ObstacleData)
		{
			UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s: ObstacleData가 비어 있어 건너뜀"), *Label);
			continue;
		}

		FIntPoint Coord;
		if (!WorldToCoord(Marker->GetActorLocation(), Coord))
		{
			UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s: 그리드 밖 %s, 건너뜀"), *Label, *Coord.ToString());
			continue;
		}
		if (Walls.Contains(Coord))
		{
			UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s: 벽 칸 %s, 건너뜀"), *Label, *Coord.ToString());
			continue;
		}
		if (Used.Contains(Coord))
		{
			UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s: %s에 이미 장애물이 있어 건너뜀"), *Label, *Coord.ToString());
			continue;
		}
		if (SpawnTiles.Contains(Coord))
		{
			UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s: 스폰 칸 %s 위에 장애물 (굽긴 함)"), *Label, *Coord.ToString());
		}

		Used.Add(Coord);
		FObstacleSpawnInfo& Info = TileMap->Obstacles.AddDefaulted_GetRef();
		Info.Coordinate = Coord;
		Info.ObstacleData = Marker->ObstacleData;
		Info.Rotation = Marker->GetActorRotation();
		Info.Scale = Marker->GetActorScale3D();
	}
}

void AStageBaker::LoadSlots(const UStageDataAsset* Stage)
{
	for (const FPropSlotInfo& Slot : Stage->PropSlots)
	{
		if (AStagePropMarker* Marker = SpawnMarker<AStagePropMarker>(Slot.Transform))
		{
			Marker->SlotType = Slot.SlotType;
			Marker->Mesh = Slot.Mesh;
			Marker->RerunConstructionScripts();   // 코드로 바꾼 필드는 OnConstruction이 자동으로 안 돎
		}
	}

	for (const FEntitySlotInfo& Slot : Stage->EntitySlots)
	{
		if (AStageEntityMarker* Marker = SpawnMarker<AStageEntityMarker>(Slot.Transform))
		{
			Marker->SlotType = Slot.SlotType;
			Marker->ActorClass = Slot.ActorClass;
			Marker->RerunConstructionScripts();
		}
	}

	for (const FExitSlotInfo& Slot : Stage->ExitSlots)
	{
		if (AStageExitMarker* Marker = SpawnMarker<AStageExitMarker>(Slot.Transform))
		{
			Marker->Position = Slot.Position;
			Marker->Mesh = Slot.Mesh;
			Marker->RerunConstructionScripts();
		}
	}
}

void AStageBaker::LoadObstacles(const UTileMapDataAsset* TileMap)
{
	// DA에 저장된 그리드 기준으로 되돌린다 → 구웠던 자리에 정확히
	if (!TileMap->GridOrigin.Equals(GetActorLocation()) || !FMath::IsNearlyEqual(TileMap->TileSize, TileSize))
	{
		UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s는 다른 그리드(원점 %s, 타일 %.0f)로 구워졌다. 다음 Bake 때 이 베이커 그리드로 다시 맞춰짐."),
			*TileMap->GetName(), *TileMap->GridOrigin.ToCompactString(), TileMap->TileSize);
	}

	for (const FObstacleSpawnInfo& Info : TileMap->Obstacles)
	{
		const FVector Location = TileMap->GridOrigin + FVector(Info.Coordinate.X * TileMap->TileSize, Info.Coordinate.Y * TileMap->TileSize, 0.0f);
		const FTransform Transform(Info.Rotation, Location, Info.Scale);

		if (AGridObstacleMarker* Marker = SpawnMarker<AGridObstacleMarker>(Transform))
		{
			Marker->ObstacleData = Info.ObstacleData;
			Marker->RerunConstructionScripts();
		}
	}
}

TMap<FIntPoint, ETileType> AStageBaker::CollectSpawnTiles(bool bLogWarnings) const
{
	TMap<FIntPoint, ETileType> Result;

	const TArray<AGridSpawnVolume*> Volumes = GatherMarkers<AGridSpawnVolume>();
	if (Volumes.IsEmpty())
	{
		return Result;
	}

	TArray<int32> CoveredCount;
	CoveredCount.SetNumZeroed(Volumes.Num());

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const FIntPoint Coord(X, Y);
			const FVector Center = CoordToWorld(Coord);

			bool bAlly = false;
			bool bEnemy = false;
			for (int32 i = 0; i < Volumes.Num(); ++i)
			{
				if (Volumes[i]->ContainsXY(Center))
				{
					++CoveredCount[i];
					(Volumes[i]->Team == ESpawnVolumeTeam::Ally ? bAlly : bEnemy) = true;
				}
			}

			if (bAlly && bEnemy)
			{
				if (bLogWarnings)
				{
					UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s: 아군/적 볼륨이 겹침, 건너뜀"), *Coord.ToString());
				}
				continue;
			}
			if (bAlly || bEnemy)
			{
				Result.Add(Coord, bAlly ? ETileType::SpawnPoint : ETileType::EnemySpawnPoint);
			}
		}
	}

	if (bLogWarnings)
	{
		for (int32 i = 0; i < Volumes.Num(); ++i)
		{
			if (CoveredCount[i] == 0)
			{
				UE_LOG(LogSP, Warning, TEXT("[StageBaker] %s: 칸 중심을 하나도 안 덮음 (그리드 밖이거나 너무 작음)"), *Volumes[i]->GetActorLabel());
			}
		}
	}
	return Result;

}

void AStageBaker::BakeSpawns(UTileMapDataAsset* TileMap) const
{
	const TSet<FIntPoint> Walls(TileMap->WallTiles);
	int32 AllyCount = 0;
	int32 EnemyCount = 0;

	TileMap->SpecialTiles.Reset();
	for (const TPair<FIntPoint, ETileType>& Pair : CollectSpawnTiles(/*bLogWarnings*/ true))
	{
		if (Walls.Contains(Pair.Key))
		{
			UE_LOG(LogSP, Warning, TEXT("[StageBaker] 스폰 칸 %s가 벽이라 건너뜀"), *Pair.Key.ToString());
			continue;
		}

		FTileSpawnInfo& Info = TileMap->SpecialTiles.AddDefaulted_GetRef();
		Info.Coordinate = Pair.Key;
		Info.TileType = Pair.Value;
		(Pair.Value == ETileType::SpawnPoint ? AllyCount : EnemyCount)++;
	}

	if (AllyCount == 0 || EnemyCount == 0)
	{
		UE_LOG(LogSP, Warning, TEXT("[StageBaker] 스폰 칸 부족: 아군 %d / 적 %d"), AllyCount, EnemyCount);
	}
}

void AStageBaker::LoadSpawns(const UTileMapDataAsset* TileMap)
{
	// 영역 정보는 DA에 없으니 칸마다 1타일 볼륨으로 복원
	for (const FTileSpawnInfo& Info : TileMap->SpecialTiles)
	{
		if (Info.TileType == ETileType::Normal)
		{
			continue;
		}

		const FVector Location = TileMap->GridOrigin + FVector(Info.Coordinate.X * TileMap->TileSize, Info.Coordinate.Y * TileMap->TileSize, 0.0f);
		if (AGridSpawnVolume* Volume = SpawnMarker<AGridSpawnVolume>(FTransform(Location)))
		{
			Volume->Team = (Info.TileType == ETileType::SpawnPoint) ? ESpawnVolumeTeam::Ally : ESpawnVolumeTeam::Enemy;
			Volume->RerunConstructionScripts();          // 색 갱신
			Volume->FitToSingleTile(TileMap->TileSize);  // Rerun 뒤에 (네이티브 컴포넌트 값이 리셋될 일은 없다고 보지만 순서로 안전하게)
		}
	}
}


template<typename T>
TArray<T*> AStageBaker::GatherMarkers() const
{
	// 이 베이커와 같은 레벨의 마커만 (서브레벨이 섞여도 남의 마커를 안 건드림)
	TArray<T*> Result;
	if (const ULevel* Level = GetLevel())
	{
		for (AActor* Actor : Level->Actors)
		{
			T* Marker = Cast<T>(Actor);
			if (IsValid(Marker))
			{
				Result.Add(Marker);
			}
		}
	}
	return Result;
}

template<typename T>
T* AStageBaker::SpawnMarker(const FTransform& Transform)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.OverrideLevel = GetLevel();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	T* Marker = World->SpawnActor<T>(T::StaticClass(), Transform, Params);
	if (Marker)
	{
		Marker->SetFolderPath(FName(TEXT("StageMarkers")));
	}
	return Marker;
}
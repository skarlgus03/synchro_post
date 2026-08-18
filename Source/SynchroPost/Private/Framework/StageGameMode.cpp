#include "Framework/StageGameMode.h"
#include "Framework/RunProgressSubsystem.h"
#include "Framework/GridManager.h"
#include "GameFlow/StageDataAsset.h"
#include "Unit/Unit.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Algo/RandomShuffle.h"
#include "Framework/SynchroPostSettings.h"
#include "Framework/SPPlayerState.h"
#include "Slot/UnitSlot.h"
#include "Slot/UnitSlotComponent.h"
#include "Framework/SPGameState.h"
#include "Framework/TurnManager.h"

void AStageGameMode::AssembleCurrentStage()
{
	URunProgressSubsystem* RunProgress = GetGameInstance()->GetSubsystem<URunProgressSubsystem>();
	if (!RunProgress || !RunProgress->ResolvedNodeGraph.IsValidIndex(RunProgress->CurrentNodeIndex))
	{
		return;
	}

	const FStageNode& Node = RunProgress->ResolvedNodeGraph[RunProgress->CurrentNodeIndex];
	UStageDataAsset* StageData = Node.StageData.LoadSynchronous();
	if (!StageData) return;

	AssembleCommon(StageData);

	if (const UCombatStageDataAsset* CombatStageData = Cast<UCombatStageDataAsset>(StageData))
	{
		AssembleCombat(Node, CombatStageData);
	}
}

void AStageGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	SpawnPartyForPlayer(NewPlayer);

	// 테스트용 — 나중에 삭제
	if (URunProgressSubsystem* RunProgress = GetGameInstance()->GetSubsystem<URunProgressSubsystem>())
	{
		if (RunProgress->ResolvedNodeGraph.Num() == 0) // 멀티플레이 대비: 이미 생성됐으면 재생성 안 함
		{
			RunProgress->GenerateFloor(TestFloorDataAsset);
			RunProgress->EnterNode(0);
		}
	}
}

void AStageGameMode::SpawnPartyForPlayer(APlayerController* NewPlayer)
{
	if (!NewPlayer) return;

	ASPPlayerState* PS = NewPlayer->GetPlayerState<ASPPlayerState>();
	if (!PS || !PS->GetUnitSlotComponent()) return;

	const USynchroPostSettings* Settings = GetDefault<USynchroPostSettings>();

	int32 SlotIndex = 0;
	for (const TSoftObjectPtr<UUnitDataAsset>& UnitDataPtr : Settings->TestParty)
	{
		UUnitDataAsset* UnitData = UnitDataPtr.LoadSynchronous();
		if (!UnitData) continue;

		UUnitSlot* Slot = PS->GetUnitSlotComponent()->GetUnitSlotByIndex(SlotIndex);
		if (!Slot) continue;

		AUnit* NewUnit = GetWorld()->SpawnActor<AUnit>(AUnit::StaticClass());
		if (NewUnit)
		{
			NewUnit->InitializeUnit(UnitData);
			Slot->SetUnit(NewUnit);
		}
		++SlotIndex;
	}
}

void AStageGameMode::AssembleCommon(const UStageDataAsset* StageData)
{
	SpawnProps(StageData);
	SpawnEntities(StageData);
	SpawnExits(StageData);
}

void AStageGameMode::AssembleCombat(const FStageNode& Node, const UCombatStageDataAsset* CombatStageData)
{
	UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();
	if (!GridManager) return;

	GridManager->LoadGrid(CombatStageData->TileMap);
	SpawnEnemies(GridManager, Node.ResolvedEnemyComposition);
	PlaceAllyUnits(GridManager);
}

void AStageGameMode::SpawnProps(const UStageDataAsset* StageData)
{
	for (const FPropSlotInfo& Slot : StageData->PropSlots)
	{
		UStaticMesh* Mesh = Slot.Mesh.LoadSynchronous();
		if (!Mesh) continue;

		AStaticMeshActor* PropActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Slot.Transform);
		if (PropActor)
		{
			PropActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		}
	}
}


void AStageGameMode::SpawnEntities(const UStageDataAsset* StageData)
{
	for (const FEntitySlotInfo& Slot : StageData->EntitySlots)
	{
		UClass* EntityClass = Slot.ActorClass.LoadSynchronous();
		if (!EntityClass) continue;

		GetWorld()->SpawnActor<AActor>(EntityClass, Slot.Transform);
	}
}

void AStageGameMode::SpawnExits(const UStageDataAsset* StageData)
{
	for (const FExitSlotInfo& Slot : StageData->ExitSlots)
	{
		UStaticMesh* Mesh = Slot.Mesh.LoadSynchronous();

		if (!Mesh) continue;

		AStaticMeshActor*  ExitActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Slot.Transform);
		if (ExitActor)
		{
			ExitActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		}
	}
}

TArray<AUnit*> AStageGameMode::SpawnEnemies(UGridManager* GridManager, const TArray<FEnemySpawnInfo>& Composition)
{
	TArray<AUnit*> SpawnedEnemies;
	TArray<FIntPoint> SpawnCoords = GridManager->GetTilesOfType(ETileType::EnemySpawnPoint);
	Algo::RandomShuffle(SpawnCoords);

	if (SpawnCoords.Num() < Composition.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("적 스폰 포인트(%d)가 스폰할 적 수(%d)보다 적음"), SpawnCoords.Num(), Composition.Num());
	}

	int32 SpawnIndex = 0;
	for (const FEnemySpawnInfo& Enemy : Composition)
	{
		if (!Enemy.UnitData || !SpawnCoords.IsValidIndex(SpawnIndex)) continue;

		const FIntPoint& Coord = SpawnCoords[SpawnIndex];
		AUnit* EnemyUnit = GetWorld()->SpawnActor<AUnit>(AUnit::StaticClass(), FTransform(GridManager->GetTileWorldLocation(Coord)));
		if (EnemyUnit)
		{
			EnemyUnit->InitializeUnit(Enemy.UnitData);
			GridManager->SetUnitAt(Coord, EnemyUnit);
			SpawnedEnemies.Add(EnemyUnit);
		}
		++SpawnIndex;
	}
	return SpawnedEnemies;
}

TArray<AUnit*> AStageGameMode::PlaceAllyUnits(UGridManager* GridManager)
{
	TArray<FIntPoint> SpawnCoords = GridManager->GetTilesOfType(ETileType::SpawnPoint);
	Algo::RandomShuffle(SpawnCoords);

	ASPGameState* SPGameState = GetGameState<ASPGameState>();
	if (!SPGameState) return {};

	// 모든 플레이어의 슬롯에서 유닛을 가져와서 아군 유닛 리스트를 생성
	TArray<AUnit*> AllyUnits;
	for (APlayerState* PS : SPGameState->PlayerArray)
	{
		ASPPlayerState* SPPlayerState = Cast<ASPPlayerState>(PS);
		if (!SPPlayerState || !SPPlayerState->GetUnitSlotComponent()) continue;

		for (UUnitSlot* Slot : SPPlayerState->GetUnitSlotComponent()->GetUnitSlots())
		{
			if (Slot && Slot->GetCurrentUnit())
			{
				AllyUnits.Add(Slot->GetCurrentUnit());
			}
		}
	}

	if (SpawnCoords.Num() < AllyUnits.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("아군 스폰 포인트(%d)가 아군 수(%d)보다 적음"), SpawnCoords.Num(), AllyUnits.Num());
	}

	// 아군 유닛을 스폰 포인트에 배치
	int32 Index = 0;
	for (AUnit* Ally : AllyUnits)
	{
		if (!SpawnCoords.IsValidIndex(Index)) break;
		GridManager->SetUnitAt(SpawnCoords[Index], Ally);
		++Index;
	}
	return AllyUnits;
}

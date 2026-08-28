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
#include "Framework/SPPlayerController.h"
#include "Types/SynchroPostTypes.h"

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

	ULevel* StageLevel = RunProgress->GetCurrentStageLevel();

	AssembleCommon(StageData, StageLevel);

	if (const UCombatStageDataAsset* CombatStageData = Cast<UCombatStageDataAsset>(StageData))
	{
		AssembleCombat(Node, CombatStageData, StageLevel);
	}
}

void AStageGameMode::SendCurrentStageDataToPlayer(ASPPlayerController* PC)
{
	
	if (!PC || PC->IsLocalController())
	{
		return;
	}

	URunProgressSubsystem* RunProgress = GetGameInstance()->GetSubsystem<URunProgressSubsystem>();
	if (!RunProgress || !RunProgress->ResolvedNodeGraph.IsValidIndex(RunProgress->CurrentNodeIndex))
	{
		return;
	}

	const FStageNode& Node = RunProgress->ResolvedNodeGraph[RunProgress->CurrentNodeIndex];
	UStageDataAsset* StageData = Node.StageData.LoadSynchronous();
	if (!StageData)
	{
		return;
	}

	PC->Client_LoadStageLevel(StageData->Level);

	if (const UCombatStageDataAsset* CombatStageData = Cast<UCombatStageDataAsset>(StageData))
	{
		PC->Client_InitializeCombatGrid(CombatStageData->TileMap);
	}
}

void AStageGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);


	if (URunProgressSubsystem* RunProgress = GetGameInstance()->GetSubsystem<URunProgressSubsystem>())
	{
		if (RunProgress->ResolvedNodeGraph.Num() == 0)
		{
			GetWorldTimerManager().SetTimerForNextTick(this, &AStageGameMode::StartTestRun);
		}
	}
}

void AStageGameMode::StartTestRun()
{
	SpawnPartyForRun();

	if (URunProgressSubsystem* RunProgress = GetGameInstance()->GetSubsystem<URunProgressSubsystem>())
	{
		RunProgress->GenerateFloor(TestFloorDataAsset);
		RunProgress->EnterNode(0);
	}
}

void AStageGameMode::SpawnPartyForRun()
{
	ASPGameState* SPGameState = GetGameState<ASPGameState>();
	if (!SPGameState || !SPGameState->GetUnitSlotComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPartyForRun: GameState 또는 UnitSlotComponent가 없음"));
		return;
	}

	SPGameState->GetUnitSlotComponent()->EnsureSlotsInitialized();

	const USynchroPostSettings* Settings = GetDefault<USynchroPostSettings>();
	UE_LOG(LogTemp, Log, TEXT("SpawnPartyForRun: TestParty 개수=%d, UnitSlots 개수=%d"),
		Settings->TestParty.Num(), SPGameState->GetUnitSlotComponent()->GetUnitSlots().Num());

	int32 SlotIndex = 0;
	for (const TSoftObjectPtr<UUnitDataAsset>& UnitDataPtr : Settings->TestParty)
	{
		UUnitDataAsset* UnitData = UnitDataPtr.LoadSynchronous();
		if (!UnitData)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnPartyForRun: [%d] UnitData 로드 실패"), SlotIndex);
			continue;
		}

		UUnitSlot* Slot = SPGameState->GetUnitSlotComponent()->GetUnitSlotByIndex(SlotIndex);
		if (!Slot)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnPartyForRun: [%d] 슬롯이 없음"), SlotIndex);
			continue;
		}

		AUnit* NewUnit = GetWorld()->SpawnActor<AUnit>(AUnit::StaticClass());
		UE_LOG(LogTemp, Log, TEXT("SpawnPartyForRun: [%d] SpawnActor 결과=%s"), SlotIndex, NewUnit ? *NewUnit->GetName() : TEXT("NULL"));

		if (NewUnit)
		{
			NewUnit->InitializeUnit(UnitData);
			NewUnit->SetFaction(EFaction::Player);
			Slot->SetUnit(NewUnit);
		}
		++SlotIndex;
	}
}

void AStageGameMode::AssembleCommon(const UStageDataAsset* StageData, ULevel* StageLevel)
{
	SpawnProps(StageData, StageLevel);
	SpawnEntities(StageData, StageLevel);
	SpawnExits(StageData, StageLevel);
}

void AStageGameMode::AssembleCombat(const FStageNode& Node, const UCombatStageDataAsset* CombatStageData, ULevel* StageLevel)
{
	UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();
	if (!GridManager) return;

	UE_LOG(LogTemp, Log, TEXT("LoadGrid: TileMap"));
	GridManager->LoadGrid(CombatStageData->TileMap);
	if (ASPGameState* SPGameState = GetGameState<ASPGameState>())
	{
		for (APlayerState* PS : SPGameState->PlayerArray)
		{
			if (ASPPlayerController* PC = PS ? Cast<ASPPlayerController>(PS->GetPlayerController()) : nullptr)
			{
				PC->Client_InitializeCombatGrid(CombatStageData->TileMap);
			}
		}
	}

	TArray<AUnit*> Allies = PlaceAllyUnits(GridManager);
	TArray<AUnit*> Enemies = SpawnEnemies(GridManager, Node.ResolvedEnemyComposition, StageLevel);
	

	TArray<AUnit*> AllParticipants;
	AllParticipants.Append(Enemies);
	AllParticipants.Append(Allies);

	if (UTurnManager* TurnManager = GetWorld()->GetSubsystem<UTurnManager>())
	{
		TurnManager->OnCombatEnd.AddUniqueDynamic(this, &AStageGameMode::HandleCombatEnd);
		TurnManager->StartCombat(AllParticipants);
	}
}

void AStageGameMode::SpawnProps(const UStageDataAsset* StageData, ULevel* StageLevel)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.OverrideLevel = StageLevel;

	for (const FPropSlotInfo& Slot : StageData->PropSlots)
	{
		UStaticMesh* Mesh = Slot.Mesh.LoadSynchronous();
		if (!Mesh) continue;

		AStaticMeshActor* PropActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Slot.Transform, SpawnParams);
		if (PropActor)
		{
			PropActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		}
	}
}

void AStageGameMode::SpawnEntities(const UStageDataAsset* StageData, ULevel* StageLevel)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.OverrideLevel = StageLevel;

	for (const FEntitySlotInfo& Slot : StageData->EntitySlots)
	{
		UClass* EntityClass = Slot.ActorClass.LoadSynchronous();
		if (!EntityClass) continue;

		GetWorld()->SpawnActor<AActor>(EntityClass, Slot.Transform, SpawnParams);
	}
}

void AStageGameMode::SpawnExits(const UStageDataAsset* StageData, ULevel* StageLevel)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.OverrideLevel = StageLevel;

	for (const FExitSlotInfo& Slot : StageData->ExitSlots)
	{
		UStaticMesh* Mesh = Slot.Mesh.LoadSynchronous();
		if (!Mesh) continue;

		AStaticMeshActor* ExitActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Slot.Transform, SpawnParams);
		if (ExitActor)
		{
			ExitActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
		}
	}
}

TArray<AUnit*> AStageGameMode::SpawnEnemies(UGridManager* GridManager, const TArray<FEnemySpawnInfo>& Composition, ULevel* StageLevel)
{
	TArray<AUnit*> SpawnedEnemies;
	TArray<FIntPoint> SpawnCoords = GridManager->GetTilesOfType(ETileType::EnemySpawnPoint);
	Algo::RandomShuffle(SpawnCoords);

	if (SpawnCoords.Num() < Composition.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("적 스폰 포인트(%d)가 스폰할 적 수(%d)보다 적음"), SpawnCoords.Num(), Composition.Num());
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.OverrideLevel = StageLevel;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	int32 SpawnIndex = 0;
	for (const FEnemySpawnInfo& Enemy : Composition)
	{
		if (!Enemy.UnitData || !SpawnCoords.IsValidIndex(SpawnIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnEnemies: 인덱스 %d 스킵 (UnitData=%s, CoordValid=%s)"),
				SpawnIndex, Enemy.UnitData ? TEXT("있음") : TEXT("NULL"),
				SpawnCoords.IsValidIndex(SpawnIndex) ? TEXT("true") : TEXT("false"));

			continue;
		}
		const FIntPoint& Coord = SpawnCoords[SpawnIndex];
		AUnit* EnemyUnit = GetWorld()->SpawnActor<AUnit>(AUnit::StaticClass(), FTransform(GridManager->GetTileWorldLocation(Coord)), SpawnParams);

		UE_LOG(LogTemp, Log, TEXT("SpawnEnemies: [%d] SpawnActor 결과=%s, Coord=(%d,%d)"),
			SpawnIndex, EnemyUnit ? *EnemyUnit->GetName() : TEXT("NULL"), Coord.X, Coord.Y);


		if (EnemyUnit)
		{
			EnemyUnit->InitializeUnit(Enemy.UnitData);
			EnemyUnit->SetFaction(EFaction::Enemy);
			GridManager->SetUnitAt(Coord, EnemyUnit);
			SpawnedEnemies.Add(EnemyUnit);
		}
		++SpawnIndex;
	}
	
	UE_LOG(LogTemp, Log, TEXT("SpawnEnemies: 최종 스폰된 적 수=%d"), SpawnedEnemies.Num());


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
	for (UUnitSlot* Slot : SPGameState->GetUnitSlotComponent()->GetUnitSlots())
	{
		if (Slot && Slot->GetCurrentUnit())
		{
			AllyUnits.Add(Slot->GetCurrentUnit());
		}
	}


	if (SpawnCoords.Num() < AllyUnits.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("아군 스폰 포인트(%d)가 아군 수(%d)보다 적음"), SpawnCoords.Num(), AllyUnits.Num());
	}

	int32 Index = 0;
	for (AUnit* Ally : AllyUnits)
	{
		if (!SpawnCoords.IsValidIndex(Index)) break;
		Ally->SetActorLocation(GridManager->GetTileWorldLocation(SpawnCoords[Index]));
		GridManager->SetUnitAt(SpawnCoords[Index], Ally);
		++Index;
	}
	return AllyUnits;
}

void AStageGameMode::HandleCombatEnd(ECombatResult Result)
{
	if (Result == ECombatResult::Victory)
	{
		UE_LOG(LogTemp, Log, TEXT("전투 승리! 다음 노드를 선택하세요."));

		if (ASPGameState* SPGameState = GetGameState<ASPGameState>())
		{
			for (APlayerState* PS : SPGameState->PlayerArray)
			{
				if (ASPPlayerController* PC = PS ? Cast<ASPPlayerController>(PS->GetPlayerController()) : nullptr)
				{
					PC->Client_ShowNodeSelection();
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("전투 패배..."));
		// TODO: 런 종료 처리
	}
}


#include "Framework/RunProgressSubsystem.h"
#include "GameFlow/FloorDataAsset.h"
#include "GameFlow/SpawnTableDataAsset.h"
#include "Framework/SynchroPostSettings.h"
#include "GameFlow/StageDataAsset.h"

void URunProgressSubsystem::GenerateFloor(UFloorDataAsset* Floor)
{
	if (!Floor)
	{
		UE_LOG(LogTemp, Warning, TEXT("GenerateFloor: Floor is null"));
		return;
	}

	ResolvedNodeGraph.Empty();
	CurrentFloor = Floor;

	// 노드 껍데기 + 연결관계 생성
	ResolvedNodeGraph = GenerateTopology(Floor->GenerationConfig);
	
	// 특정 층 강제 타입 지정	(보스, 이벤트 등) + 나머지 가중치 랜덤 배정
	AssignStageTypes(ResolvedNodeGraph, Floor->GenerationConfig);

	// Combat/Elite 노드의 실제 스폰 결과 확정
	for (FStageNode& Node : ResolvedNodeGraph)
	{
		if (Node.StageType == EStageType::Combat || Node.StageType == EStageType::Elite)
		{
			ResolveEncounter(Node, Floor);
		}
	}

	// Boss 노드 별도 생성해서 그래프 끝에 이어붙인다.
	FStageNode BossNode;
	BossNode.StageType = EStageType::Boss;
	BossNode.Depth = Floor->GenerationConfig.StageCount; // 명시적으로 설정 — 마지막 층 다음 단계
	ResolveBossEncounter(BossNode, Floor);

	int32 BossIndex = ResolvedNodeGraph.Add(BossNode);
	for (FStageNode& Node : ResolvedNodeGraph)
	{
		if (Node.Depth == Floor->GenerationConfig.StageCount - 1)
		{
			Node.NextNodeIndices.Add(BossIndex);
		}
	}

	CurrentFloorIndex++;
	CurrentNodeIndex = 0;
}

int32 URunProgressSubsystem::CalculateEncounterBudget(EStageType Type) const
{
	const USynchroPostSettings* Settings = GetDefault<USynchroPostSettings>();
	const float DifficultyMultiplier = Settings->DifficultySettings[CurrentDifficulty].BudgetMultiplier;

	int32 BaseBudget = Settings->BaseCombatBudget;
	if (Type == EStageType::Elite)
	{
		BaseBudget = Settings->BaseEliteBudget;
	}
	else if (Type == EStageType::Boss)
	{
		BaseBudget = Settings->BaseBossEscortBudget;
	}

	return (BaseBudget + CurrentFloorIndex * Settings->BudgetPerFloor) * DifficultyMultiplier;
}

void URunProgressSubsystem::DebugPrintFloor() const
{
	for (int32 Depth = 0; Depth <= CurrentFloor->GenerationConfig.StageCount; ++Depth)
	{
		UE_LOG(LogTemp, Log, TEXT("=== Depth %d ==="), Depth);

		for (int32 i = 0; i < ResolvedNodeGraph.Num(); ++i)
		{
			const FStageNode& Node = ResolvedNodeGraph[i];
			if (Node.Depth != Depth) continue;

			FString EnemyList;
			for (const FEnemySpawnInfo& Enemy : Node.ResolvedEnemyComposition)
			{
				EnemyList += Enemy.UnitData ? Enemy.UnitData.GetName() + TEXT(", ") : TEXT("NULL, ");
			}

			UE_LOG(LogTemp, Log, TEXT(" [%d] Type=%s Next=%s Enemies=[%s]"),
				i, *UEnum::GetValueAsString(Node.StageType),
				*FString::JoinBy(Node.NextNodeIndices, TEXT(","),
					[](int32 Idx) { return FString::FromInt(Idx); }), *EnemyList);
		}
	}
}

void URunProgressSubsystem::ValidateFloorGraph() const
{
	const int32 LastDepth = CurrentFloor->GenerationConfig.StageCount - 1;

	for (const FStageNode& Node : ResolvedNodeGraph)
	{
		// 막다른 길 체크 (마지막 층 제외)
		if (Node.StageType != EStageType::Boss && Node.Depth < LastDepth && Node.NextNodeIndices.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[검증 실패] Depth %d 노드에 나가는 간선이 없음"), Node.Depth);
		}

		// 마지막 층은 전부 Rest여야 함
		if (Node.Depth == LastDepth && Node.StageType != EStageType::Rest)
		{
			UE_LOG(LogTemp, Error, TEXT("[검증 실패] 마지막 층인데 Rest가 아님: %s"),
				*UEnum::GetValueAsString(Node.StageType));
		}

		// Combat/Elite인데 적이 하나도 없으면
		if ((Node.StageType == EStageType::Combat || Node.StageType == EStageType::Elite )
			&& Node.ResolvedEnemyComposition.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[검증 실패] Depth %d 전투 노드에 적이 0마리"), Node.Depth);
		}
	}
}

TArray<FStageNode> URunProgressSubsystem::GenerateTopology(const FFloorGenerationConfig& Config)
{
	TArray<FStageNode> Nodes;
	TArray<TArray<int32>> LayerNodeIndices; // 층별 실제 배열 인덱스 

	// 노드 격자 생성
	for (int32 Depth = 0; Depth < Config.StageCount; ++Depth)
	{
		int32 NodeCount = (Depth == 0) ? 1 : Config.LaneCount;
		TArray<int32> ThisLayer;

		for (int32 Lane = 0; Lane < NodeCount; ++Lane)
		{
			FStageNode NewNode;
			NewNode.Depth = Depth;
			ThisLayer.Add(Nodes.Add(NewNode));
		}
		LayerNodeIndices.Add(ThisLayer);
	}

	// 2. 층마다 간선 연결
	for (int32 Depth = 0; Depth < Config.StageCount - 1; ++Depth)
	{
		const TArray<int32>& CurrentLayer = LayerNodeIndices[Depth];
		const TArray<int32>& NextLayer = LayerNodeIndices[Depth + 1];

		// 2-1. 일단 각 노드마다 1~3개 랜덤 연결
		for (int32 CurLane = 0; CurLane < CurrentLayer.Num(); ++CurLane)
		{
			int32 ConnectionCount = FMath::RandRange(1, 3);
			for (int32 c = 0; c < ConnectionCount; ++c)
			{
				int32 TargetLane = FMath::Clamp(CurLane + FMath::RandRange(-1, 1), 0, NextLayer.Num() - 1);
				Nodes[CurrentLayer[CurLane]].NextNodeIndices.AddUnique(NextLayer[TargetLane]);
			}
		}

		// 2-2. 다음 층에서 "들어오는 간선 없는" 노드 찾아서 강제 연결
		for (int32 NextLane : NextLayer)
		{
			bool bHasIncoming = CurrentLayer.ContainsByPredicate([&](int32 Idx) {
				return Nodes[Idx].NextNodeIndices.Contains(NextLane);
				});

			if (!bHasIncoming)
			{
				int32 RandomSource = CurrentLayer[FMath::RandRange(0, CurrentLayer.Num() - 1)];
				Nodes[RandomSource].NextNodeIndices.AddUnique(NextLane);
			}
		}

		// 2-3. 현재 층에서 "나가는 간선 없는" 노드(막다른 길) 찾아서 강제 연결
		for (int32 CurLane : CurrentLayer)
		{
			if (Nodes[CurLane].NextNodeIndices.Num() == 0)
			{
				int32 RandomTarget = NextLayer[FMath::RandRange(0, NextLayer.Num() - 1)];
				Nodes[CurLane].NextNodeIndices.Add(RandomTarget);
			}
		}
	}

	return Nodes;

}

void URunProgressSubsystem::AssignStageTypes(TArray<FStageNode>& Nodes, const FFloorGenerationConfig& Config)
{
	const int32 RestDepth = Config.StageCount - 1;
	const bool bForceDraftAtStart = (CurrentFloorIndex > 0);

	int32 UpgradeDepth;

	do
	{
		UpgradeDepth = FMath::RandRange(0, Config.StageCount - 1);
	} while (UpgradeDepth == RestDepth || (bForceDraftAtStart && UpgradeDepth == 0));

	for (FStageNode& Node : Nodes)
	{
		if (bForceDraftAtStart && Node.Depth == 0)
		{
			Node.StageType = EStageType::Draft;
		}
		else if (Node.Depth == RestDepth)
		{
			Node.StageType = EStageType::Rest;
		}
		else if (Node.Depth == UpgradeDepth)
		{
			Node.StageType = EStageType::Upgrade;
		}
		else
		{
			Node.StageType = RollRandomStageType(Config);
		}
	}
}

void URunProgressSubsystem::ResolveEncounter(FStageNode& Node, UFloorDataAsset* Floor)
{
	const TArray<TObjectPtr<USpawnTableDataAsset>>& Pool =
		(Node.StageType == EStageType::Elite) ? Floor->ElitePool : Floor->NormalPool;

	if (Pool.Num() == 0 || Floor->CombatArenaPool.Num() == 0) return;

	USpawnTableDataAsset* ChosenTable = Pool[FMath::RandRange(0, Pool.Num() - 1)];
	Node.StageData = Floor->CombatArenaPool[FMath::RandRange(0, Floor->CombatArenaPool.Num() - 1)];

	const int32 Budget = CalculateEncounterBudget(Node.StageType); // Depth → StageType
	Node.ResolvedEnemyComposition = RollSpawnTableUntilBudgetSpent(ChosenTable->SpawnList, Budget);
}

void URunProgressSubsystem::ResolveBossEncounter(FStageNode& BossNode, UFloorDataAsset* Floor)
{
	if (Floor->BossPool.Num() == 0) return;

	UBossSpawnTableDataAsset* ChosenBoss = Floor->BossPool[FMath::RandRange(0, Floor->BossPool.Num() - 1)];
	if (!ChosenBoss) return;

	BossNode.StageData = ChosenBoss->StageTemplate;

	FEnemySpawnInfo BossEntry;
	BossEntry.UnitData = ChosenBoss->Boss;
	BossNode.ResolvedEnemyComposition.Add(BossEntry);

	const int32 EscortBudget = CalculateEncounterBudget(EStageType::Boss); // Depth → 고정 Boss
	TArray<FEnemySpawnInfo> Escorts = RollSpawnTableUntilBudgetSpent(ChosenBoss->SpawnList, EscortBudget);
	BossNode.ResolvedEnemyComposition.Append(Escorts);
}

EStageType URunProgressSubsystem::RollRandomStageType(const FFloorGenerationConfig& Config)
{
	if (FMath::FRand() < Config.CombatStageRatio)
	{
		const USynchroPostSettings* Settings = GetDefault<USynchroPostSettings>();
		const float EliteChance = Settings->DifficultySettings[CurrentDifficulty].EliteChance;
		return (FMath::FRand() < EliteChance) ? EStageType::Elite : EStageType::Combat;
	}
	return PickWeightedNonCombatType(Config.NonCombatWeights);
}

EStageType URunProgressSubsystem::PickWeightedNonCombatType(const TMap<EStageType, float>& Weights)
{
	// 전체 가중치 합 구하기
	float TotalWeight = 0.0f;

	for (const auto& Pair : Weights)
	{
		TotalWeight += Pair.Value;
	}

	if (TotalWeight <= 0.0f)
	{
		return EStageType::Event; // 기본값
	}

	// 0 ~ TotalWeight 범위에서 랜덤 값 생성
	const float Roll = FMath::FRandRange(0.0f, TotalWeight);

	// 맵을 순회하며 누적값이 Roll을 넘는 순간 그 항목이 당첨
	float Accumulated = 0.0f;

	for (const auto& Pair : Weights)
	{
		Accumulated += Pair.Value;
		if (Roll <= Accumulated)
		{
			return Pair.Key;
		}
	}
	return EStageType::Event; // 기본값
}

TArray<FEnemySpawnInfo> URunProgressSubsystem::RollSpawnTableUntilBudgetSpent(const TMap<TObjectPtr<UUnitDataAsset>, int32>& SpawnList, int32 Budget)
{
	TArray<FEnemySpawnInfo> Result;
	if (SpawnList.Num() == 0)
	{
		return Result;
	}

	int32 RemainingBudget = Budget;
	int32 SafetyCounter = 0; // 안전장치: 무한루프 방지

	while (RemainingBudget > 0 && SafetyCounter < 100)
	{
		++SafetyCounter;

		// 가중치 기반 랜덤 선택
		float TotalWeight = 0.0f;
		for (const auto& Pair : SpawnList)
		{
			TotalWeight += 1.0f / FMath::Max(Pair.Value, 1);
		}

		const float Roll = FMath::FRandRange(0.0f, TotalWeight);
		float Accumulated = 0.0f;

		// 선택된 유닛이 남은 예산을 초과하면 선택하지 않고 루프를 계속
		for (const auto& Pair : SpawnList)
		{
			Accumulated += 1.0f / FMath::Max(Pair.Value, 1);
			if (Roll <= Accumulated)
			{
				FEnemySpawnInfo NewEntry;
				NewEntry.UnitData = Pair.Key;
				Result.Add(NewEntry);
				RemainingBudget -= Pair.Value; // 예산 초과해도 이번 픽은 인정하고 종료
				break;
			}
		}
	}
	return Result;
}

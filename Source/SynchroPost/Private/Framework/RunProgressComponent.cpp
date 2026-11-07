
#include "Framework/RunProgressComponent.h"
#include "GameFlow/FloorDataAsset.h"
#include "GameFlow/SpawnTableDataAsset.h"
#include "Framework/SynchroPostSettings.h"
#include "GameFlow/StageDataAsset.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Framework/StageGameMode.h"
#include "Framework/SPPlayerController.h"
#include "Framework/SPGameState.h"
#include "Net/UnrealNetwork.h"
#include "SynchroPost.h"
#include "Unit/Unit.h"
#include "Framework/SPPlayerState.h"
#include "Framework/TurnManager.h"
#include "GameFramework/GameStateBase.h"


URunProgressComponent::URunProgressComponent()
{
	SetIsReplicatedByDefault(true);
}

URunProgressComponent* URunProgressComponent::Get(const UObject* WorldContext)
{
	if (!WorldContext) return nullptr;
	if (UWorld* World = WorldContext->GetWorld())
	{
		if (ASPGameState* GS = World->GetGameState<ASPGameState>())
		{
			return GS->GetRunProgressComponent();
		}
	}
	return nullptr;
}

void URunProgressComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URunProgressComponent, RunPhase);
	DOREPLIFETIME(URunProgressComponent, StageLoadSerial);
	DOREPLIFETIME(URunProgressComponent, StageLevel);
	DOREPLIFETIME(URunProgressComponent, ReachableNodes);
	DOREPLIFETIME(URunProgressComponent, StageParticipants);
}

void URunProgressComponent::SetRunPhase(ERunPhase NewPhase)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || RunPhase == NewPhase)
	{
		return;
	}

	RunPhase = NewPhase;

	// 노드 선택 단계에서만 갈 수 있는 노드를 보여준다
	ReachableNodes.Reset();
	if (NewPhase == ERunPhase::NodeSelect)
	{
		for (int32 NodeIndex : GetReachableNodeIndices())
		{
			FNodeChoice& Choice = ReachableNodes.AddDefaulted_GetRef();
			Choice.NodeIndex = NodeIndex;
			Choice.StageType = ResolvedNodeGraph[NodeIndex].StageType;
		}
	}

	// 서버는 직접 호출함
	OnRep_RunPhase();
}

void URunProgressComponent::GenerateFloor(UFloorDataAsset* Floor)
{
	if (!Floor)
	{
		UE_LOG(LogSP, Warning, TEXT("GenerateFloor: Floor is null"));
		return;
	}

	ResolvedNodeGraph.Empty();
	CurrentFloor = Floor;

	// 노드 껍데기 + 연결관계 생성
	ResolvedNodeGraph = GenerateTopology(Floor->GenerationConfig);
	
	// 특정 층 강제 타입 지정	(이벤트 등) + 나머지 가중치 랜덤 배정
	AssignStageTypes(ResolvedNodeGraph, Floor->GenerationConfig);

	// Combat/Elite 노드의 실제 스폰 결과 확정
	for (FStageNode& Node : ResolvedNodeGraph)
	{
		if (Node.StageType == EStageType::Combat || Node.StageType == EStageType::Elite)
		{
			ResolveEncounter(Node, Floor);
		}
		else
		{
			ResolveNonCombatStageData(Node, Floor);
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

void URunProgressComponent::EnterNode(int32 NodeIndex)
{
	if (!GetWorld() || !GetWorld()->GetAuthGameMode())
	{
		UE_LOG(LogSP, Warning, TEXT("EnterNode: Server authority not found (or World is null)"));
		return;
	}

	if (!ResolvedNodeGraph.IsValidIndex(NodeIndex))
	{
		UE_LOG(LogSP, Warning, TEXT("EnterNode: Wrong NodeIndex(%d), ResolvedNodeGraph.Num()=%d"), NodeIndex, ResolvedNodeGraph.Num());
		return;
	}

	CurrentNodeIndex = NodeIndex;

	const FStageNode& Node = ResolvedNodeGraph[NodeIndex];
	UStageDataAsset* LoadedStageData = Node.StageData.LoadSynchronous();

	if (!LoadedStageData)
	{
		UE_LOG(LogSP, Warning, TEXT("EnterNode: Node[%d](Type=%s) has no StageData"), NodeIndex, *UEnum::GetValueAsString(Node.StageType));
		return;
	}

	if (LoadedStageData->Level.IsNull())
	{
		UE_LOG(LogSP, Warning, TEXT("EnterNode: Node[%d]'s StageData(%s) has no Level assigned"), NodeIndex, *LoadedStageData->GetName());
		return;
	}

	// 스테이지 진입 시점에 조립 상태 초기화
	bStageAssembled = false;
	StageParticipants.Reset();

	// 서버에서만 StageLoadSerial을 증가시킴
	++StageLoadSerial;
	StageLevel = LoadedStageData->Level;
	SetRunPhase(ERunPhase::Deploy);
	StreamInStageLevel(LoadedStageData->Level, StageLoadSerial);

	if (!CurrentStageLevel)
	{
		UE_LOG(LogSP, Error, TEXT("EnterNode: 서브레벨 스트리밍 시작 실패"));
		return;
	}

	UE_LOG(LogSP, Log, TEXT("EnterNode: Node[%d] 진입, 서브레벨 로드 시작"), NodeIndex);

}

void URunProgressComponent::RequestEnterNode(const APlayerController* Requester, int32 StageSerial, int32 NodeIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const TCHAR* RejectReason = nullptr;
	if (StageSerial != StageLoadSerial)
	{
		RejectReason = TEXT("스테이지 번호 불일치 (이전 스테이지의 늦은 요청)");
	}
	else if (RunPhase != ERunPhase::NodeSelect)
	{
		RejectReason = TEXT("노드 선택 단계가 아님");
	}
	else if (!CanChooseNode(Requester))
	{
		RejectReason = TEXT("결정권 없음");
	}
	else if (!ReachableNodes.ContainsByPredicate([NodeIndex](const FNodeChoice& Choice) { return Choice.NodeIndex == NodeIndex; }))
	{
		RejectReason = TEXT("갈 수 없는 노드");
	}

	if (RejectReason)
	{
		UE_LOG(LogSP, Warning, TEXT("[RunProgress] 노드 진입 거부: %s | 요청자=%s Serial=%d(현재 %d) Node=%d"),
			RejectReason, *GetNameSafe(Requester), StageSerial, StageLoadSerial, NodeIndex);
		return;
	}

	EnterNode(NodeIndex);
}

int32 URunProgressComponent::CalculateEncounterBudget(EStageType Type) const
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

bool URunProgressComponent::CanChooseNode(const APlayerController* PC)
{
	// 방장 = 서버에 있는 로컬
	return PC && PC->IsLocalController() && PC->HasAuthority();
}

void URunProgressComponent::DebugPrintFloor() const
{
	for (int32 Depth = 0; Depth <= CurrentFloor->GenerationConfig.StageCount; ++Depth)
	{
		UE_LOG(LogSP, Log, TEXT("=== Depth %d ==="), Depth);

		for (int32 i = 0; i < ResolvedNodeGraph.Num(); ++i)
		{
			const FStageNode& Node = ResolvedNodeGraph[i];
			if (Node.Depth != Depth) continue;

			FString EnemyList;
			for (const FEnemySpawnInfo& Enemy : Node.ResolvedEnemyComposition)
			{
				EnemyList += Enemy.UnitData ? Enemy.UnitData.GetName() + TEXT(", ") : TEXT("NULL, ");
			}

			UE_LOG(LogSP, Log, TEXT(" [%d] Type=%s Next=%s Enemies=[%s]"),
				i, *UEnum::GetValueAsString(Node.StageType),
				*FString::JoinBy(Node.NextNodeIndices, TEXT(","),
					[](int32 Idx) { return FString::FromInt(Idx); }), *EnemyList);
		}
	}
}

void URunProgressComponent::ValidateFloorGraph() const
{
	const int32 LastDepth = CurrentFloor->GenerationConfig.StageCount - 1;

	for (const FStageNode& Node : ResolvedNodeGraph)
	{
		// 막다른 길 체크 (마지막 층 제외)
		if (Node.StageType != EStageType::Boss && Node.Depth < LastDepth && Node.NextNodeIndices.Num() == 0)
		{
			UE_LOG(LogSP, Error, TEXT("[검증 실패] Depth %d 노드에 나가는 간선이 없음"), Node.Depth);
		}

		// 마지막 층은 전부 Rest여야 함
		if (Node.Depth == LastDepth && Node.StageType != EStageType::Rest)
		{
			UE_LOG(LogSP, Error, TEXT("[검증 실패] 마지막 층인데 Rest가 아님: %s"),
				*UEnum::GetValueAsString(Node.StageType));
		}

		// Combat/Elite인데 적이 하나도 없으면
		if ((Node.StageType == EStageType::Combat || Node.StageType == EStageType::Elite )
			&& Node.ResolvedEnemyComposition.Num() == 0)
		{
			UE_LOG(LogSP, Error, TEXT("[검증 실패] Depth %d 전투 노드에 적이 0마리"), Node.Depth);
		}
	}
}

ULevel* URunProgressComponent::GetCurrentStageLevel() const
{
	return CurrentStageLevel ? CurrentStageLevel->GetLoadedLevel() : nullptr;
}

TArray<int32> URunProgressComponent::GetReachableNodeIndices() const
{
	if (!ResolvedNodeGraph.IsValidIndex(CurrentNodeIndex))
	{
		return {};
	}

	return ResolvedNodeGraph[CurrentNodeIndex].NextNodeIndices;
}

void URunProgressComponent::OnRep_RunPhase()
{
	UE_LOG(LogSP, Log, TEXT("[RunProgress] RunPhase → %s (%s), ReachableNodes=%d"),
		*UEnum::GetValueAsString(RunPhase),
		GetOwner() && GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"),
		ReachableNodes.Num());

	OnRunPhaseChanged.Broadcast(RunPhase);

	RefreshLocalDeployReady();
}

void URunProgressComponent::OnRep_StageLoadSerial()
{
	UE_LOG(LogSP, Log, TEXT("[RunProgress] StageLoadSerial → %d (Client)"), StageLoadSerial);

	// 이미 같은 스테이지를 불러왔으면 무시
	if (StageLoadSerial <= 0 || StageLoadSerial == LocallyLoadedSerial)
	{
		return;
	}
	if (StageLevel.IsNull())
	{
		UE_LOG(LogSP, Warning, TEXT("[RunProgress] StageLoadSerial %d 인데 StageLevel이 비어 있음"), StageLoadSerial);
		return;
	}

	// 서버와 같은 이름(CombatStage_<번호>)으로 불러와야 레벨 속 액터가 복제로 짝지어진다
	StreamInStageLevel(StageLevel, StageLoadSerial);
	RefreshLocalDeployReady();
}

void URunProgressComponent::RefreshLocalDeployReady()
{
	bool bAllArrived = StageParticipants.Num() > 0;
	for (const AUnit* Unit : StageParticipants)
	{
		if (!Unit)
		{
			bAllArrived = false;	// 아직 이 기기에 안 온 유닛
			break;
		}
	}

	// 준비 가능 조건: Deploy 단계, 스테이지 로딩 완료, 참가자 전원 도착
	const bool bNewCanReady =
		RunPhase == ERunPhase::Deploy
		&& StageLoadSerial > 0	 
		&& LocallyShownSerial == StageLoadSerial // 스테이지 로딩 완료
		&& bAllArrived;
	
	if (bNewCanReady == bLocalCanReady)
	{
		return;
	}
	bLocalCanReady = bNewCanReady;

	UE_LOG(LogSP, Log, TEXT("[RunProgress] 준비 가능 %s (%s) Stage=%d"),
		bLocalCanReady ? TEXT("O") : TEXT("X"),
		GetOwner() && GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"),
		StageLoadSerial);

	OnLocalDeployReadyChanged.Broadcast(bLocalCanReady);
}

TArray<FStageNode> URunProgressComponent::GenerateTopology(const FFloorGenerationConfig& Config)
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

void URunProgressComponent::AssignStageTypes(TArray<FStageNode>& Nodes, const FFloorGenerationConfig& Config)
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

void URunProgressComponent::ResolveEncounter(FStageNode& Node, UFloorDataAsset* Floor)
{
	const TArray<TObjectPtr<USpawnTableDataAsset>>& Pool =
		(Node.StageType == EStageType::Elite) ? Floor->ElitePool : Floor->NormalPool;

	if (Pool.Num() == 0 || Floor->CombatArenaPool.Num() == 0) return;

	USpawnTableDataAsset* ChosenTable = Pool[FMath::RandRange(0, Pool.Num() - 1)];
	Node.StageData = Floor->CombatArenaPool[FMath::RandRange(0, Floor->CombatArenaPool.Num() - 1)];

	const int32 Budget = CalculateEncounterBudget(Node.StageType); // Depth → StageType
	Node.ResolvedEnemyComposition = RollSpawnTableUntilBudgetSpent(ChosenTable->SpawnList, Budget);
}

void URunProgressComponent::ResolveBossEncounter(FStageNode& BossNode, UFloorDataAsset* Floor)
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

void URunProgressComponent::ResolveNonCombatStageData(FStageNode& Node, UFloorDataAsset* Floor)
{
	UStageDataAsset* ChosenStage = nullptr;

	switch (Node.StageType)
	{
	case EStageType::Rest:
		if (Floor->RestStagePool.Num() > 0)
		{
			ChosenStage = Floor->RestStagePool[FMath::RandRange(0, Floor->RestStagePool.Num() - 1)];
		}
		break;
	case EStageType::Shop:
		if (Floor->ShopStagePool.Num() > 0)
		{
			ChosenStage = Floor->ShopStagePool[FMath::RandRange(0, Floor->ShopStagePool.Num() - 1)];
		}
		break;
	case EStageType::Upgrade:
		if (Floor->UpgradeStagePool.Num() > 0)
		{
			ChosenStage = Floor->UpgradeStagePool[FMath::RandRange(0, Floor->UpgradeStagePool.Num() - 1)];
		}			
		break;
	case EStageType::Draft:
		if (Floor->DraftStagePool.Num() > 0)
		{
			ChosenStage = Floor->DraftStagePool[FMath::RandRange(0, Floor->DraftStagePool.Num() - 1)];
		}
		break;
	case EStageType::Event:
		if (Floor->EventStagePool.Num() > 0)
		{
			ChosenStage = Floor->EventStagePool[FMath::RandRange(0, Floor->EventStagePool.Num() - 1)];
		}
		break;
	default:
		break;
	}

	if (ChosenStage)
	{
		Node.StageData = TSoftObjectPtr<UStageDataAsset>(ChosenStage);
	}
}

EStageType URunProgressComponent::RollRandomStageType(const FFloorGenerationConfig& Config)
{
	if (FMath::FRand() < Config.CombatStageRatio)
	{
		const USynchroPostSettings* Settings = GetDefault<USynchroPostSettings>();
		const float EliteChance = Settings->DifficultySettings[CurrentDifficulty].EliteChance;
		return (FMath::FRand() < EliteChance) ? EStageType::Elite : EStageType::Combat;
	}
	return PickWeightedNonCombatType(Config.NonCombatWeights);
}

EStageType URunProgressComponent::PickWeightedNonCombatType(const TMap<EStageType, float>& Weights)
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

TArray<FEnemySpawnInfo> URunProgressComponent::RollSpawnTableUntilBudgetSpent(const TMap<TObjectPtr<UUnitDataAsset>, int32>& SpawnList, int32 Budget)
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

void URunProgressComponent::HandleStageLevelShown()
{
	LocallyShownSerial = StageLoadSerial;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (AStageGameMode* StageGameMode = Cast<AStageGameMode>(GetWorld()->GetAuthGameMode()))
		{
			StageGameMode->AssembleCurrentStage();
		}
	}

	RefreshLocalDeployReady();
}

void URunProgressComponent::StreamInStageLevel(const TSoftObjectPtr<UWorld>& LevelAsset, int32 Serial)
{
	// 이전 스테이지는 언로드 "요청"만 된다 (비동기). 다음 틱까지 스트리밍 목록에 남아 있으므로
	// 새 스테이지는 다른 이름이어야 한다
	if (CurrentStageLevel)
	{
		CurrentStageLevel->SetIsRequestingUnloadAndRemoval(true);
		CurrentStageLevel = nullptr;
	}

	const FString InstanceName = FString::Printf(TEXT("CombatStage_%d"), Serial);

	bool bSuccess = false;
	CurrentStageLevel = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		this, LevelAsset, FVector::ZeroVector, FRotator::ZeroRotator, bSuccess, InstanceName);
	if (!bSuccess || !CurrentStageLevel)
	{
		UE_LOG(LogSP, Error, TEXT("StreamInStageLevel: 서브레벨 로드 실패 (%s)"), *InstanceName);
	}
	LocallyLoadedSerial = Serial;

	CurrentStageLevel->OnLevelShown.AddDynamic(this, &URunProgressComponent::HandleStageLevelShown);
}

void URunProgressComponent::SetStageParticipants(const TArray<AUnit*>& Participants)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	StageParticipants.Reset();
	for (AUnit* Unit : Participants)
	{
		StageParticipants.Add(Unit);
	}
	bStageAssembled = true;

	RefreshLocalDeployReady();

	TryStartCombat();
}

void URunProgressComponent::RequestSetReady(const APlayerController* Requester, int32 StageSerial, bool bReady)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ASPPlayerState* PS = Requester ? Requester->GetPlayerState<ASPPlayerState>() : nullptr;

	const TCHAR* RejectReason = nullptr;
	if (!PS)
	{
		RejectReason = TEXT("PlayerState 없음");
	}
	else if (StageSerial != StageLoadSerial)
	{
		RejectReason = TEXT("스테이지 번호 불일치");
	}
	else if (RunPhase != ERunPhase::Deploy)
	{
		RejectReason = TEXT("배치 단계가 아님");
	}

	if (RejectReason)
	{
		UE_LOG(LogSP, Warning, TEXT("[RunProgress] 준비 요청 거부: %s | 요청자=%s Serial=%d(현재 %d)"),
			RejectReason, *GetNameSafe(Requester), StageSerial, StageLoadSerial);
		return;
	}

	PS->SetReadyStageSerial(bReady ? StageSerial : 0);
	UE_LOG(LogSP, Log, TEXT("[RunProgress] %s 준비 %s (Stage %d)"),
		*GetNameSafe(PS), bReady ? TEXT("완료") : TEXT("취소"), StageSerial);

	if (bReady)
	{
		TryStartCombat();
	}
}

bool URunProgressComponent::AreAllPlayersReady() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState || GameState->PlayerArray.Num() == 0)
	{
		return false;
	}
	// PlayerArray는 접속 중인 플레이어만 담는다 → 나간 사람은 자동 제외, 배치 중 들어온 사람은 자동 포함
	for (const APlayerState* PS : GameState->PlayerArray)
	{
		const ASPPlayerState* SPPS = Cast<ASPPlayerState>(PS);
		if (!SPPS || SPPS->GetReadyStageSerial() != StageLoadSerial)
		{
			return false;
		}
	}
	return true;
}

void URunProgressComponent::TryStartCombat()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (RunPhase != ERunPhase::Deploy || !bStageAssembled || !AreAllPlayersReady())
	{
		return;
	}

	UTurnManager* TurnManager = GetWorld()->GetSubsystem<UTurnManager>();
	if (!TurnManager)
	{
		return;
	}

	TArray<AUnit*> Participants;
	for (AUnit* Unit : StageParticipants)
	{
		if (Unit)
		{
			Participants.Add(Unit);
		}
	}

	SetRunPhase(ERunPhase::Combat);	// 먼저 바꿔서 같은 프레임에 다시 불려도 두 번 시작하지 않게
	TurnManager->StartCombat(Participants);
}

void URunProgressComponent::OnRep_StageParticipants()
{
	// 2번 조각에서 "유닛이 다 도착했나" 확인에 쓴다. 지금은 로그만
	int32 Resolved = 0;
	for (const AUnit* Unit : StageParticipants)
	{
		Resolved += Unit ? 1 : 0;
	}
	UE_LOG(LogSP, Log, TEXT("[RunProgress] StageParticipants %d/%d 도착 (Client)"), Resolved, StageParticipants.Num());

	RefreshLocalDeployReady();
}
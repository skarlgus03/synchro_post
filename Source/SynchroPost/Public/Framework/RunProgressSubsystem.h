#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Types/SPGameFlowStructure.h"
#include "RunProgressSubsystem.generated.h"

class UFloorDataAsset;
class ULevelStreamingDynamic;
class ULevel;

UCLASS()
class SYNCHROPOST_API URunProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	int32 CurrentFloorIndex = 0;

	UPROPERTY()
	TObjectPtr<UFloorDataAsset> CurrentFloor;

	// 절차 생성된 노드 그래프를 저장하는 배열
	UPROPERTY()
	TArray<FStageNode> ResolvedNodeGraph;

	UPROPERTY()
	int32 CurrentNodeIndex = 0;

	UPROPERTY()
	EGameDifficulty CurrentDifficulty = EGameDifficulty::Normal;

public:

	// FloorDA 를 기반으로 노드 그래프를 절차 생성한다.
	UFUNCTION(BlueprintCallable, Category = "Run Progress")
	void GenerateFloor(UFloorDataAsset* Floor);

	UFUNCTION(BlueprintCallable, Category = "Run Progress")
	void EnterNode(int32 NodeIndex);

	// 현재 스테이지의 예산을 계산한다.
	int32 CalculateEncounterBudget(EStageType Type) const;

	// 디버그용으로 현재 층의 노드 그래프를 출력한다.
	UFUNCTION(BlueprintCallable, Category = "Run Progress|Debug")
	void DebugPrintFloor() const;

	// 디버그용으로 현재 층의 노드 그래프들을 검증한다.
	UFUNCTION(BlueprintCallable, Category = "Run Progress|Debug")
	void ValidateFloorGraph() const;

	// 현재 스테이지 서브레벨을 반환한다. 없으면 nullptr 반환
	ULevel* GetCurrentStageLevel() const;

	// 현재 노드에서 이동 가능한 노드들의 인덱스를 반환한다.
	UFUNCTION(BlueprintCallable, Category = "Run Progress")
	TArray<int32> GetReachableNodeIndices() const;

private:

	// 현재 스트리밍중인 스테이지 서브레벨
	UPROPERTY()
	TObjectPtr<ULevelStreamingDynamic> CurrentStageLevel;

private:

	// 층/레인 격자 노드 생성 + 인접 층 끼리 간선 연결
	TArray<FStageNode> GenerateTopology(const FFloorGenerationConfig& Config);
	
	// 특정 층 강제 타입 지정	(보스, 이벤트 등) + 나머지 가중치 랜덤 배정
	void AssignStageTypes(TArray<FStageNode>& Nodes, const FFloorGenerationConfig& Config);

	// Combat/Elite 노드의 실제 스폰 결과 확정
	void ResolveEncounter(FStageNode& Node, UFloorDataAsset* Floor);

	// Boss 노드 별도 처리
	void ResolveBossEncounter(FStageNode& BossNode, UFloorDataAsset* Floor);

	// 전투 스테이지가 아닌 노드의 스테이지 데이터 확정
	void ResolveNonCombatStageData(FStageNode& Node, UFloorDataAsset* Floor);

	// 스테이지의 종류를 랜덤으로 결정한다
	EStageType RollRandomStageType(const FFloorGenerationConfig& Config);

	// 가중치에 따라 전투 스테이지가 아닌곳을 랜덤으로 결정한다
	EStageType PickWeightedNonCombatType(const TMap<EStageType, float>& Weights);

	// 스폰 테이블을 기반으로 예산이 소진될 때까지 유닛을 랜덤으로 뽑는다.
	TArray<FEnemySpawnInfo> RollSpawnTableUntilBudgetSpent(const TMap<TObjectPtr<UUnitDataAsset>, int32>& SpawnList, int32 Budget);

	UFUNCTION()
	void HandleStageLevelShown();
};

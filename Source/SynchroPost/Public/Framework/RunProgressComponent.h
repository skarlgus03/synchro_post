#pragma once

#include "CoreMinimal.h"
#include "Types/SPGameFlowStructure.h"
#include "Components/ActorComponent.h"
#include "RunProgressComponent.generated.h"

class UFloorDataAsset;
class ULevelStreamingDynamic;
class ULevel;
class ASPPlayerController;
class AUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunPhaseChanged, ERunPhase, NewPhase);

UCLASS()
class SYNCHROPOST_API URunProgressComponent : public UActorComponent
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

	// 단계가 바뀔 때 (서버: SetRunPhase, 클라: OnRep). 모든 기기에서 한 번씩 불린다
	UPROPERTY(BlueprintAssignable)
	FOnRunPhaseChanged OnRunPhaseChanged;

public:

	URunProgressComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;



	// Run Progress
	
	// FloorDA 를 기반으로 노드 그래프를 절차 생성한다.
	UFUNCTION(BlueprintCallable, Category = "Run Progress")
	void GenerateFloor(UFloorDataAsset* Floor);

	UFUNCTION(BlueprintCallable, Category = "Run Progress")
	void EnterNode(int32 NodeIndex);

	// 서버 전용. 플레이어의 노드 진입 요청을 검증하고, 통과하면 진입한다
	void RequestEnterNode(const APlayerController* Requester, int32 StageSerial, int32 NodeIndex);

	// 현재 스테이지의 예산을 계산한다.
	int32 CalculateEncounterBudget(EStageType Type) const;

	// 이 플레이어가 노드를 고를 수 있는가? (지금 방장만 됨. 나중에 투표기능 나오면 확장)
	static bool CanChooseNode(const APlayerController* PC);
	
		

	// 서버 전용. 조립이 끝난 스테이지의 참가자를 등록하고, 이미 전원 준비면 바로 시작
	void SetStageParticipants(const TArray<AUnit*>& Participants);

	// 서버 전용. 플레이어의 준비/취소 요청을 검증하고 반영
	void RequestSetReady(const APlayerController* Requester, int32 StageSerial, bool bReady);

	// 서버 전용. 조립 완료 + 접속 중인 전원 준비면 전투 시작 (준비 변경, 조립 완료, 플레이어 퇴장 때 호출)
	void TryStartCombat();

	const TArray<TObjectPtr<AUnit>>& GetStageParticipants() const { return StageParticipants; }

	

	// 디버그용으로 현재 층의 노드 그래프를 출력한다.
	UFUNCTION(BlueprintCallable, Category = "Run Progress|Debug")
	void DebugPrintFloor() const;

	// 디버그용으로 현재 층의 노드 그래프들을 검증한다.
	UFUNCTION(BlueprintCallable, Category = "Run Progress|Debug")
	void ValidateFloorGraph() const;


	// ---- Getters / Setters ----


	// WorldContext를 기반으로 URunProgressComponent를 반환한다. 없으면 nullptr 반환 
	static URunProgressComponent* Get(const UObject* WorldContext);


	ERunPhase GetRunPhase() const { return RunPhase; }
	
	// 서버에서만 호출. 클라에서는 OnRep_RunPhase()로 RunPhase를 갱신한다.
	void SetRunPhase(ERunPhase NewPhase);

	// 현재 스테이지 서브레벨을 반환한다. 없으면 nullptr 반환
	ULevel* GetCurrentStageLevel() const;

	// 현재 노드에서 이동 가능한 노드들의 인덱스를 반환한다.
	UFUNCTION(BlueprintCallable, Category = "Run Progress")
	TArray<int32> GetReachableNodeIndices() const;

	// 스테이지 서브레벨 번호. 인스턴스 이름에 쓴다 (서버와 클라가 같은 이름이어야 레벨 속 액터가 복제로 짝지어진다)
	int32 GetStageLoadSerial() const { return StageLoadSerial; }

	const TArray<FNodeChoice>& GetReachableNodes() const { return ReachableNodes; }
	
private:

	// 현재 스트리밍중인 스테이지 서브레벨
	UPROPERTY()
	TObjectPtr<ULevelStreamingDynamic> CurrentStageLevel;

	// 현재 RunPhase. 복제됨. 서버만 SetRunPhase()로 바꿈 클라는 OnRep_RunPhase() 호출
	UPROPERTY(ReplicatedUsing = OnRep_RunPhase)
	ERunPhase RunPhase = ERunPhase::None;

	// 서버가 스테이지를 불러올 때마다 1씩 올림.
	UPROPERTY(ReplicatedUsing = OnRep_StageLoadSerial)
	int32 StageLoadSerial = 0;

	// 현재 스테이지 레벨 (클라이언트가 OnRep으로 불러올 때 사용)
	UPROPERTY(Replicated)
	TSoftObjectPtr<UWorld> StageLevel;

	UFUNCTION()
	void OnRep_RunPhase();

	UFUNCTION()
	void OnRep_StageLoadSerial();

	// 이 기기가 마지막으로 불러온 스테이지 번호 (복제 안 함)
	int32 LocallyLoadedSerial = 0;

	// NodeSelect 단계에서만 채워진다. 갈 수 있는 노드들의 정보 배열
	UPROPERTY(Replicated)
	TArray<FNodeChoice> ReachableNodes;

	// 이번 스테이지 참가 유닛. 클라가 "유닛이 다 도착했나"를 확인하는 데 쓴다 
	UPROPERTY(ReplicatedUsing = OnRep_StageParticipants)
	TArray<TObjectPtr<AUnit>> StageParticipants;

	UFUNCTION()
	void OnRep_StageParticipants();

	// 서버 전용. 이번 스테이지 조립이 끝났는가
	bool bStageAssembled = false;

	bool AreAllPlayersReady() const;

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

	void StreamInStageLevel(const TSoftObjectPtr<UWorld>& LevelAsset, int32 Serial);
};

#pragma once

#include "CoreMinimal.h"
#include "Framework/SPGameMode.h"
#include "Types/SPGameFlowStructure.h"
#include "StageGameMode.generated.h"

class UStageDataAsset;
class UCombatStageDataAsset;
class UGridManager;
class AUnit;
class UFloorDataAsset;
enum class ECombatResult : uint8;

UCLASS()
class SYNCHROPOST_API AStageGameMode : public ASPGameMode
{
	GENERATED_BODY()

public:

	// 스트리밍으로 새 스테이지가 로드된 직후 RunProgressSubsystem이 호출함
	UFUNCTION(BlueprintCallable, Category = "Stage")
	void AssembleCurrentStage();

protected:

	virtual void PostLogin(APlayerController* NewPlayer) override;


	UPROPERTY(EditAnywhere, Category = "Test")
	TObjectPtr<UFloorDataAsset> TestFloorDataAsset;

private:

	// 플레이어가 접속하면 파티를 스폰하고, 플레이어의 UnitSlotComponent에 연결
	void SpawnPartyForPlayer(APlayerController* NewPlayer);

	// 프롭, 엔티티, 출구 등 공통적인 스테이지 구성 요소를 배치
	void AssembleCommon(const UStageDataAsset* StageData, ULevel* StageLevel);
	// 전투 스테이지에만 해당하는 구성 요소를 배치
	void AssembleCombat(const FStageNode& Node, const UCombatStageDataAsset* CombatStageData, ULevel* StageLevel);

	void SpawnProps(const UStageDataAsset* StageData, ULevel* StageLevel);
	void SpawnEntities(const UStageDataAsset* StageData, ULevel* StageLevel);
	void SpawnExits(const UStageDataAsset* StageData, ULevel* StageLevel);
	TArray<AUnit*> SpawnEnemies(UGridManager* GridManager, const TArray<FEnemySpawnInfo>& Composition, ULevel* StageLevel);

	TArray<AUnit*> PlaceAllyUnits(UGridManager* GridManager);

	UFUNCTION()
	void HandleCombatEnd(ECombatResult Result);
};
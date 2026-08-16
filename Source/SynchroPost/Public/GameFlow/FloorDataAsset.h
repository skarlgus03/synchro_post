
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/SPGameFlowStructure.h"
#include "FloorDataAsset.generated.h"

class USpawnTableDataAsset;
class UBossSpawnTableDataAsset;
class UCombatStageDataAsset;
class URestStageDataAsset;
class UShopStageDataAsset;
class UUpgradeStageDataAsset;
class UDraftStageDataAsset;
class UEventStageDataAsset;

UCLASS()
class SYNCHROPOST_API UFloorDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
	
	// 노드 그래프 생성 규칙
	UPROPERTY(EditAnywhere, Category = "Generation")
	FFloorGenerationConfig GenerationConfig;
	

	// 몬스터 스폰 테이블 풀

	UPROPERTY(EditAnywhere, Category = "Floor|SpawnTable")
	TArray<TObjectPtr<USpawnTableDataAsset>> NormalPool;

	UPROPERTY(EditAnywhere, Category = "Floor|SpawnTable")
	TArray<TObjectPtr<USpawnTableDataAsset>> ElitePool;
	
	UPROPERTY(EditAnywhere, Category = "Floor|SpawnTable")
	TArray<TObjectPtr<UBossSpawnTableDataAsset>> BossPool;


	// 스테이지 데이터 에셋 풀

	UPROPERTY(EditAnywhere, Category = "Floor|Stage")
	TArray<TObjectPtr<UCombatStageDataAsset>> CombatArenaPool;

	UPROPERTY(EditAnywhere, Category = "Floor|Stage")
	TArray<TObjectPtr<URestStageDataAsset>>	RestStagePool;

	UPROPERTY(EditAnywhere, Category = "Floor|Stage")
	TArray<TObjectPtr<UShopStageDataAsset>>	ShopStagePool;

	UPROPERTY(EditAnywhere, Category = "Floor|Stage")
	TArray<TObjectPtr<UUpgradeStageDataAsset>> UpgradeStagePool;

	UPROPERTY(EditAnywhere, Category = "Floor|Stage")
	TArray<TObjectPtr<UDraftStageDataAsset>> DraftStagePool;

	UPROPERTY(EditAnywhere, Category = "Floor|Stage")
	TArray<TObjectPtr<UEventStageDataAsset>> EventStagePool;
};

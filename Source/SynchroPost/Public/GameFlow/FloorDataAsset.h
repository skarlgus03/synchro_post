
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/SPGameFlowStructure.h"
#include "FloorDataAsset.generated.h"

class USpawnTableDataAsset;
class UBossSpawnTableDataAsset;
class UCombatStageDataAsset;

UCLASS()
class SYNCHROPOST_API UFloorDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
	
	// 노드 그래프 생성 규칙
	UPROPERTY(EditAnywhere, Category = "Generation")
	FFloorGenerationConfig GenerationConfig;
	

	// 몬스터 스폰

	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<USpawnTableDataAsset>> NormalPool;

	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<USpawnTableDataAsset>> ElitePool;
	
	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<UBossSpawnTableDataAsset>> BossPool;


	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<UCombatStageDataAsset>> CombatArenaPool;
};

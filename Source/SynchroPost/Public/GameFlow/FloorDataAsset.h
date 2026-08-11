
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/SPGameFlowStructure.h"
#include "FloorDataAsset.generated.h"

class USpawnTableDataAsset;
class UBossSpawnTableDataAsset;

UCLASS()
class SYNCHROPOST_API UFloorDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
	// 비주얼 테마 (슬롯 타입, 실제 메시)
	UPROPERTY(EditAnywhere, Category = "Floor")
	TMap<FGameplayTag, TSoftObjectPtr<UStaticMesh>> SlotVisuals;

	// 노드 그래프 생성 규칙
	UPROPERTY(EditAnywhere, Category = "Generation")
	FStageGenerationConfig GenerationConfig;
	

	// 몬스터 스폰

	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<USpawnTableDataAsset>> NormalSpawnTables;

	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<USpawnTableDataAsset>> EliteSpawnTables;
	
	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<UBossSpawnTableDataAsset>> BossSpawnTables;
};

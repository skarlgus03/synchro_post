
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/SPGameFlowStructure.h"
#include "FloorDataAsset.generated.h"

class UItemDataAsset;
class USpawnTableDataAsset;

UCLASS()
class SYNCHROPOST_API UFloorDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<FStageNode> NodeGraph;

	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<UItemDataAsset>> ItemPool;

	UPROPERTY(EditAnywhere, Category = "Floor")
	TMap<FGameplayTag, TSoftObjectPtr<UStaticMesh>> SlotVisuals;


	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<USpawnTableDataAsset>> NormalSpawnTables;

	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<USpawnTableDataAsset>> EliteSpawnTables;
	
	UPROPERTY(EditAnywhere, Category = "Floor")
	TArray<TObjectPtr<USpawnTableDataAsset>> BossSpawnTables;
};

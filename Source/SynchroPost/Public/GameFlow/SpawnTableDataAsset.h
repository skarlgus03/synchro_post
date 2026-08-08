#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpawnTableDataAsset.generated.h"

class UUnitDataAsset;

UCLASS()
class SYNCHROPOST_API USpawnTableDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	

	// 스폰 테이블에 등록된 유닛과, 각 유닛의 코스트를 저장하는 맵
	UPROPERTY(EditAnywhere)
	TMap<TObjectPtr<UUnitDataAsset>, int32> SpawnList;
};

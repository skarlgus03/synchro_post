#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpawnTableDataAsset.generated.h"

class UUnitDataAsset;
class UCombatStageDataAsset;

UCLASS()
class SYNCHROPOST_API USpawnTableDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	

	// 스폰 테이블에 등록된 유닛과, 각 유닛의 코스트를 저장하는 맵
	UPROPERTY(EditAnywhere)
	TMap<TObjectPtr<UUnitDataAsset>, int32> SpawnList;
};

UCLASS()
class SYNCHROPOST_API UBossSpawnTableDataAsset : public USpawnTableDataAsset
{
	GENERATED_BODY()
	
public:

	// 무조건 등장하는 보스 유닛
	UPROPERTY(EditAnywhere,	Category = "Boss")
	TObjectPtr<UUnitDataAsset> Boss;

	// 보스의 전용 아레나 스테이지
	UPROPERTY(EditAnywhere,	Category = "Boss")
	TObjectPtr<UCombatStageDataAsset> StageTemplate;
};
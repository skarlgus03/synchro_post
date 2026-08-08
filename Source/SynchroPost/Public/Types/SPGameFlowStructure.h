#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SPGameFlowStructure.generated.h"


class UStageDataAsset;
class UUnitDataAsset;

UENUM(BlueprintType)
enum class EStageType : uint8
{
	Combat,
	Elite,
	Boss,
	Event,
	Rest,
	Upgrade,
	Shop,
	Draft
};

USTRUCT(BlueprintType)
struct FStageNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	EStageType StageType = EStageType::Combat;

	// 이 노드가 실제로 로드할 그리드/ 적 구성 데이터
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	TSoftObjectPtr<UStageDataAsset> StageData;

	// 다음으로 이어질 수 있는 노드들의 인덱스 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	TArray<int32> NextNodeIndices;

};

USTRUCT(BlueprintType)
struct FEnemySpawnInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Enemy")
	TObjectPtr<UUnitDataAsset> UnitData;

	// 난이도 스케일링 계수 등 (나중에 구체화)
	UPROPERTY(EditAnywhere, Category = "Enemy")
	int32 TierLevel = 0;
};

USTRUCT(BlueprintType)
struct FPropSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag SlotType;
	
	UPROPERTY(EditAnywhere)
	FTransform SlotTransform;
};

USTRUCT(BlueprintType)
struct FStageGenerationConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 StageCount = 10;

	UPROPERTY(EditAnywhere)
	float CombatStageRatio = 0.6f;

	UPROPERTY(EditAnywhere)
	int32 GuaranteedUpgradeCount = 1;
	
	UPROPERTY(EditAnywhere)
	bool bGuaranteeRestBeforeBoss = true;

};

UENUM(BlueprintType)
enum class EGameDifficulty : uint8
{
	Easy,
	Normal,
	Hard,
	Challenge
};

USTRUCT(BlueprintType)
struct FDifficultySetting
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float EliteChance = 0.0f;
};
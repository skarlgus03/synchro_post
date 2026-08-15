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
struct FEnemySpawnInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Enemy")
	TObjectPtr<UUnitDataAsset> UnitData;

	// 난이도 스케일링 계수 등 (나중에 구체화)
	UPROPERTY(EditAnywhere, Category = "Enemy")
	int32 ScalingLevel = 0;
};

// 순수 장식용 메쉬. 로직이 없고, 단순히 스테이지 디자인을 위해 배치되는 오브젝트
USTRUCT(BlueprintType)
struct FPropSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag SlotType;
	
	UPROPERTY(EditAnywhere)
	FTransform Transform;

	UPROPERTY(EditAnywhere) 
	TSoftObjectPtr<UStaticMesh> Mesh;
};

// 실제 액터로 스폰되는 로직이 있는 오브젝트. (NPC, 상호작용 오브젝트 등)
USTRUCT(BlueprintType)
struct FEntitySlotInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag SlotType;

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AActor> ActorClass;

	UPROPERTY(EditAnywhere)
	FTransform Transform;
};

USTRUCT(BlueprintType)
struct FExitSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag Position;

	UPROPERTY(EditAnywhere)
	FTransform Transform;

};

USTRUCT(BlueprintType)
struct FFloorGenerationConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 StageCount = 10;

	UPROPERTY(EditAnywhere)
	float CombatStageRatio = 0.5f;

	UPROPERTY(EditAnywhere)
	int32 GuaranteedUpgradeCount = 1;
	
	UPROPERTY(EditAnywhere)
	bool bGuaranteeRestBeforeBoss = true;

	UPROPERTY(EditAnywhere)
	int32 LaneCount = 5;

	UPROPERTY(EditAnywhere)
	TMap<EStageType, float> NonCombatWeights;

	FFloorGenerationConfig()
	{
		NonCombatWeights.Add(EStageType::Event, 10.f);
		NonCombatWeights.Add(EStageType::Rest, 5.0f);
		NonCombatWeights.Add(EStageType::Upgrade, 2.0f);
		NonCombatWeights.Add(EStageType::Shop, 5.0f);
		NonCombatWeights.Add(EStageType::Draft, 2.0f);
	}
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

	// 엘리트 스테이지가 등장할 확률
	UPROPERTY(EditAnywhere)
	float EliteChance = 0.0f;

	// 몬스터 스폰 예산에 곱해지는 계수
	UPROPERTY(EditAnywhere)
	float BudgetMultiplier = 1.0f;

	// 몬스터 스탯에 곱해지는 계수
	UPROPERTY(EditAnywhere)
	float MonsterStatMultiplier = 1.0f;

	// 특수 제약 
	UPROPERTY(EditAnywhere, meta = (Categories = "Modifier"))
	FGameplayTagContainer ModifierTags;
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

	// 이 노드의 깊이 (루트 노드가 0)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	int32 Depth = 0;

	// 다음으로 이어질 수 있는 노드들의 인덱스 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	TArray<int32> NextNodeIndices;

	UPROPERTY(BlueprintReadOnly, Category = "Stage")
	TArray<FEnemySpawnInfo> ResolvedEnemyComposition;
};
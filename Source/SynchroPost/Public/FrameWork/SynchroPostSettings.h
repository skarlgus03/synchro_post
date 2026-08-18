#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Unit/UnitStatDataAsset.h"
#include "Unit/UnitDataAsset.h"
#include "GameplayTagContainer.h"
#include "Types/SPGameFlowStructure.h"
#include "SynchroPostSettings.generated.h"



UCLASS(Config=Game, defaultconfig, meta=(DisplayName="SynchroPost Settings"))
class SYNCHROPOST_API USynchroPostSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:

	UPROPERTY(Config, EditAnywhere, Category = "Stats")
	TSoftObjectPtr<UUnitStatDataAsset> BaseStatDefaultData;

	// 상태이상의 이동포인트 페널티 수치를 정합니다. 0 = 이동불가, 50 = 절반 100 = 정상
	UPROPERTY(Config, EditAnywhere, Category = "Movement", meta = (Categories = "StatusEffect"))
	TMap<FGameplayTag, int32> DefaultMovementPenaltyTable;

	// 속도가 이동포인트로 환산되는 비율을 정합니다. 1 = 1속도당 1이동포인트, 0.5 = 1속도당 0.5이동포인트
	UPROPERTY(Config, EditAnywhere, Category = "Movement")
	float SpeedToMovePointRatio = 0.1f;


	// 난이도별 수치값 설정
	UPROPERTY(Config, EditAnywhere, Category = "Floor Generation")
	TMap<EGameDifficulty, FDifficultySetting> DifficultySettings;

	// 기본 전투 스테이지 몬스터 예산 값
	UPROPERTY(Config, EditAnywhere, Category = "Floor Generation")
	int32 BaseCombatBudget = 5;

	// 층에 따른 예산 증가량
	UPROPERTY(Config, EditAnywhere, Category = "Floor Generation")
	int32 BudgetPerFloor = 3;

	// 기본 엘리트 스테이지 몬스터 예산 값
	UPROPERTY(Config, EditAnywhere, Category = "Floor Generation")
	int32 BaseEliteBudget = 8;

	// 기본 보스 호위 몬스터 예산 값
	UPROPERTY(Config, EditAnywhere, Category = "Floor Generation")
	int32 BaseBossEscortBudget = 6;


	UPROPERTY(Config, EditAnywhere, Category = "Test")
	TArray<TSoftObjectPtr<UUnitDataAsset>> TestParty;
};


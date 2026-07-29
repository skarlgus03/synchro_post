#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Unit/UnitStatDataAsset.h"
#include "GameplayTagContainer.h"
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
};


#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ObstacleDataAsset.generated.h"

class AGridObstacle;
class UUnitHealthBarWidget;

UCLASS()
class SYNCHROPOST_API UObstacleDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText ObstacleName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<UStaticMesh> ObstacleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 MaxHealth = 100;

	// 저항만 있음 (방어력 계산 없음). 태그별 퍼센트 감소치 (0~100)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (Categories = "Damage"))
	TMap<FGameplayTag, int32> Resistances;

	// 기본값은 AGridObstacle 자체. 파괴시 특수 행동(폭발 등)이 필요할 때만 서브클래스 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
	TSubclassOf<AGridObstacle> ObstacleClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUnitHealthBarWidget> HealthBarWidgetClass;
};

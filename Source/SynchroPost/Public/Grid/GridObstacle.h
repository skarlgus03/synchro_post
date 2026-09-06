#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Damageable.h"
#include "Types/SynchroPostTypes.h"
#include "GridObstacle.generated.h"

class UObstacleDataAsset;
class UWidgetComponent;
class UUnitHealthBarWidget;
class UStaticMeshComponent;

UCLASS()
class SYNCHROPOST_API AGridObstacle : public AActor, public IDamageable
{
	GENERATED_BODY()

public:
	AGridObstacle();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서 스폰 직후 호출 - 체력/저항 초기화 (그리드 등록은 호출부에서 GridManager->SetOccupantAt으로 별도 처리)
	UFUNCTION(BlueprintCallable, Category = "Obstacle")
	void InitializeObstacle(const UObstacleDataAsset* ObstacleData);

	// IDamageable 인터페이스 구현
	virtual int32 ApplyHealthChange_Implementation(const FSPHealthActionData& ActionData) override;
	virtual int32 GetCurrentHealth_Implementation() const override;
	virtual void ApplyVisualDamage_Implementation(int32 DisplayAmount, int32 NewTargetHealth, bool bIsCritical, const FGameplayTagContainer& TypeTags) override;

	
	bool IsDestroyed() const { return bIsDestroyed; }
	FIntPoint GetGridPosition() const { return GridPosition; }
	void SetGridPosition(const FIntPoint& NewPosition) { GridPosition = NewPosition; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Obstacle")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarWidgetComponent;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Obstacle")
	int32 CurrentHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Obstacle")
	int32 MaxHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Obstacle")
	TMap<FGameplayTag, int32> Resistances;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Obstacle")
	FIntPoint GridPosition;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Obstacle")
	bool bIsDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Obstacle")
	TObjectPtr<const UObstacleDataAsset> CurrentObstacleData;

private:
	UUnitHealthBarWidget* GetHealthBarWidget() const;
};
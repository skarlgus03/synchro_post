#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Damageable.h"
#include "Types/SynchroPostTypes.h"
#include "Grid/TileMapDataAsset.h"
#include "GridObstacle.generated.h"

class UObstacleDataAsset;
class UStaticMeshComponent;
class UUnitHealthBarComponent;

UCLASS()
class SYNCHROPOST_API AGridObstacle : public AActor, public IDamageable
{
	GENERATED_BODY()

public:
	AGridObstacle();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서 스폰 직후 호출 - 체력/저항 초기화 (그리드 등록은 호출부에서 GridManager->SetOccupantAt으로 별도 처리)
	UFUNCTION(BlueprintCallable, Category = "Obstacle")
	void InitializeObstacle(const FObstacleSpawnInfo& Info);

	// IDamageable 인터페이스 구현
	virtual int32 ApplyHealthChange_Implementation(const FSPHealthActionData& ActionData) override;
	virtual int32 GetCurrentHealth_Implementation() const override;
	virtual void ApplyVisualDamage_Implementation(int32 DisplayAmount, int32 NewTargetHealth, bool bIsCritical, const FGameplayTagContainer& TypeTags) override;

	
	bool IsDestroyed() const { return bIsDestroyed; }
	FIntPoint GetGridPosition() const { return SpawnInfo.Coordinate; }
	const UObstacleDataAsset* GetObstacleData() const { return SpawnInfo.ObstacleData; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Obstacle")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Obstacle")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUnitHealthBarComponent> HealthBarWidgetComponent;

	UPROPERTY(ReplicatedUsing = OnRep_SpawnInfo)
	FObstacleSpawnInfo SpawnInfo;

	UFUNCTION()
	void OnRep_SpawnInfo();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Obstacle")
	int32 CurrentHealth = 0;

	// DA에서 각자 읽어서 복제 안해도됨
	UPROPERTY(BlueprintReadOnly, Category = "Obstacle")
	int32 MaxHealth = 0;

	// 서버 전용
	UPROPERTY(BlueprintReadOnly, Category = "Obstacle")
	TMap<FGameplayTag, int32> Resistances;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Obstacle")
	bool bIsDestroyed = false;


private:
	void ApplyVisuals();

	// 체력바에 현재 데이터를 넘긴다.
	void RefreshHealthBar();
};
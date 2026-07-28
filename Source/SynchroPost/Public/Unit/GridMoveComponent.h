

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GridMoveComponent.generated.h"

class AUnit;
class UGridManager;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API UGridMoveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGridMoveComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool RequestMove(const TArray<FIntPoint>& Path);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool CanMove() const;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetMovementPenaltyMultiplier() const;

	// 턴 시작시 호출 - 속도 비례 회복
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void RefillMovePoint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	int32 GetAvailableMovePoint() const;

	int32 GetCurrentMovePoint() const { return CurrentMovePoint; }


protected:

	UPROPERTY(ReplicatedUsing = OnRep_MovePoint)
	int32 BaseMovePoint = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MovePoint)
	int32 CurrentMovePoint = 0;

	UFUNCTION()
	void OnRep_MovePoint();

	UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (Categories = "StatusEffect"))
	TMap<FGameplayTag, float> MovementPenaltyTable;

	UPROPERTY()
	TObjectPtr<AUnit> OwnerUnit;

	UPROPERTY()
	TObjectPtr<UGridManager> CachedGridManager;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SpeedToMovePointRatio = 0.1f;
};

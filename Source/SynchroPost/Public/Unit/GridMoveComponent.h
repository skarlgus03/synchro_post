

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
	UGridMoveComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool RequestMove(const FIntPoint& Destination);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool CanMove() const;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	float GetMovementPenaltyMultiplier() const;

	// 턴 시작시 호출 - 속도 비례 회복
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void RefillMovePoint();

	// 특수 아이템/ 스킬 등이 개인 예외를 심을 때 사용
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetMovementPenaltyOverride(const FGameplayTag& Tag, float NewPenalty);


	UFUNCTION(BlueprintCallable, Category = "Movement")
	int32 GetAvailableMovePoint() const;

	int32 GetCurrentMovePoint() const { return CurrentMovePoint; }


protected:


	// 대부분 비어있다.특수 유닛만 채워진다. (전역 규칙판 보다 우선된다.)
	UPROPERTY()
	TMap<FGameplayTag, int32> PenaltyOverrides;

	UPROPERTY(ReplicatedUsing = OnRep_MovePoint)
	int32 BaseMovePoint = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MovePoint)
	int32 CurrentMovePoint = 0;

	UFUNCTION()
	void OnRep_MovePoint();


	UPROPERTY()
	TObjectPtr<AUnit> OwnerUnit;

	UPROPERTY()
	TObjectPtr<UGridManager> CachedGridManager;

};

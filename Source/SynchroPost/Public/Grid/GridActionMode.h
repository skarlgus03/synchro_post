#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GridActionMode.generated.h"

class AUnit;

UCLASS(Abstract)
class SYNCHROPOST_API UGridActionMode : public UObject
{
	GENERATED_BODY()

public:

	void Initialize(AUnit* InActingUnit) { ActingUnit = InActingUnit; }

	// 액션 모드에서 유효한 타겟 좌표를 반환하는 함수. 액션 모드에 따라 다르게 구현됨
	virtual TArray<FIntPoint> GetRangeTiles() const PURE_VIRTUAL(UGridActionMode::GetRangeTiles, return {};);
	// 액션 모드에서 유효한 타겟인지 확인하는 함수. 액션 모드에 따라 다르게 구현됨
	virtual bool IsValidTarget(const FIntPoint& HoveredCoord) const PURE_VIRTUAL(UGridActionMode::IsValidTarget, return false;);
	// 액션 모드에서 타겟 좌표에 따라 관련된 좌표들을 반환하는 함수. 액션 모드에 따라 다르게 구현됨
	virtual TArray<FIntPoint> ComputeRelatedTiles(const FIntPoint& HoveredCoord) const PURE_VIRTUAL(UGridActionMode::ComputeRelatedTiles, return {};);
	// 액션 모드에서 타겟 좌표와 관련된 좌표들을 기반으로 실제 액션을 수행하는 함수. 액션 모드에 따라 다르게 구현됨
	virtual void ConfirmAction(const FIntPoint& HoveredCoord) const PURE_VIRTUAL(UGridActionMode::ConfirmAction, );


protected:
	UPROPERTY()
	TObjectPtr<AUnit> ActingUnit;
};

UCLASS()
class SYNCHROPOST_API UMoveActionMode : public UGridActionMode
{
	GENERATED_BODY()

public:

	virtual TArray<FIntPoint> GetRangeTiles() const override;
	virtual bool IsValidTarget(const FIntPoint& HoveredCoord) const override;
	virtual TArray<FIntPoint> ComputeRelatedTiles(const FIntPoint& HoveredCoord) const override;
	virtual void ConfirmAction(const FIntPoint& HoveredCoord) const override;

};

UCLASS()
class SYNCHROPOST_API USkillActionMode : public UGridActionMode
{
	GENERATED_BODY()

public:
	virtual TArray<FIntPoint> GetRangeTiles() const override;
	virtual bool IsValidTarget(const FIntPoint& HoveredCoord) const override;
	virtual TArray<FIntPoint> ComputeRelatedTiles(const FIntPoint& HoveredCoord) const override;
	virtual void ConfirmAction(const FIntPoint& HoveredCoord) const override;

	void SetSkillSlotTag(const FGameplayTag& InSkillSlotTag) { SkillSlotTag = InSkillSlotTag; }
protected:
	UPROPERTY()
	FGameplayTag SkillSlotTag;
};
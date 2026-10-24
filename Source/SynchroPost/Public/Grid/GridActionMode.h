#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GridActionMode.generated.h"

class AUnit;
class ASPPlayerController;
struct FGridPreview;

UCLASS(Abstract)
class SYNCHROPOST_API UGridActionMode : public UObject
{
	GENERATED_BODY()

public:

	void Initialize(AUnit* InActingUnit, ASPPlayerController* InOwningController) 
	{ 
		ActingUnit = InActingUnit; 
		OwningController = InOwningController;
	}

	// 액션 모드에서 유효한 타겟 좌표를 반환하는 함수. 액션 모드에 따라 다르게 구현됨
	virtual TArray<FIntPoint> GetRangeTiles() const PURE_VIRTUAL(UGridActionMode::GetRangeTiles, return {};);
	// 액션 모드에서 유효한 타겟인지 확인하는 함수. 액션 모드에 따라 다르게 구현됨
	virtual bool IsValidTarget(const FIntPoint& HoveredCoord) const PURE_VIRTUAL(UGridActionMode::IsValidTarget, return false;);
	// 액션 모드에서 타겟 좌표에 따라 관련된 좌표들을 반환하는 함수. 액션 모드에 따라 다르게 구현됨
	virtual TArray<FIntPoint> ComputeRelatedTiles(const FIntPoint& HoveredCoord) const PURE_VIRTUAL(UGridActionMode::ComputeRelatedTiles, return {};);
	// 액션 모드에서 타겟 좌표와 관련된 좌표들을 기반으로 실제 액션을 수행하는 함수. 액션 모드에 따라 다르게 구현됨
	virtual void ConfirmAction(const FIntPoint& HoveredCoord) const PURE_VIRTUAL(UGridActionMode::ConfirmAction, );

	// 여러 번의 선택이 필요한 액션인지 확인하는 함수. 기본값은 false이며, 필요에 따라 오버라이드 가능
	virtual bool RequiresMultipleSelections() const { return false; }

	// 선택을 등록하는 함수. 기본값은 false이며, 필요에 따라 오버라이드 가능
	virtual bool RegisterSelection(const FIntPoint& SelectedCoord) { return true; }

	// 커서가 HoverCoord에 있을 때 보여줄 것을 채운다. 컨트롤러는 그대로 GridVisualizer에 넘긴다.
	// 기본: 범위 + (유효하면) 대상 칸. 자식은 경로나 효과 범위를 덧붙인다
	virtual void BuildPreview(const FIntPoint& HoverCoord, FGridPreview& Out) const;

protected:
	UPROPERTY()
	TObjectPtr<AUnit> ActingUnit;

	UPROPERTY()
	TObjectPtr<ASPPlayerController> OwningController;


	// 범위는 모드가 살아있는 동안 바뀌지 않으므로 처음 한 번만 계산함. (확정시 모드를 나감)
	const TArray<FIntPoint>& GetCachedRangeTiles() const;

private:

	// 범위 타일 캐시. 모드가 살아있는 동안 바뀌지 않음
	mutable TOptional<TArray<FIntPoint>> CachedRangeTiles;
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
	virtual void BuildPreview(const FIntPoint& HoverCoord, FGridPreview& Out) const override;
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

	virtual bool RequiresMultipleSelections() const override;
	virtual bool RegisterSelection(const FIntPoint& SelectedCoord) override;

	virtual void BuildPreview(const FIntPoint& HoverCoord, FGridPreview& Out) const override;


	void SetSkillSlotTag(const FGameplayTag& InSkillSlotTag) { SkillSlotTag = InSkillSlotTag; }
protected:
	UPROPERTY()
	FGameplayTag SkillSlotTag;

	UPROPERTY()
	TArray<FIntPoint> SelectedTiles;
};
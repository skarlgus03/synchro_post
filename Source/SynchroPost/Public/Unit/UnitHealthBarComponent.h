#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GameplayTagContainer.h"
#include "UnitHealthBarComponent.generated.h"

class AUnit;
class UUnitDataAsset;
class UUnitHealthBarWidget;

/** 체력바가 보여야 하는 '사건성' 이유들. 하나라도 켜져 있으면 보인다.
 *  각 이유는 자기만 켜고 자기만 끈다 — 한 이유가 꺼져도 다른 이유가 살아있으면 바는 유지된다.
 *
 *  '체력이 깎여 있음' 같은 지속 상태는 여기 넣지 않는다.
 *  저장하면 실제 값과 어긋나므로(힐로 풀피가 됐을 때 등) UpdateVisibility에서 매번 계산한다. */
enum class EHealthBarReason : uint8
{
	None = 0,
	RecentHit = 1 << 0,   // 방금 피격/회복됨. 타이머로 자동 해제
	Hovered = 1 << 1,   // 마우스가 올라가 있음
	Selected = 1 << 2,   // 선택됨
};
ENUM_CLASS_FLAGS(EHealthBarReason)


/**
 * 유닛 머리 위 체력바. 자기 높이와 표시 여부를 스스로 결정한다.
 *
 * AUnit은 이 컴포넌트를 소유하고 두 가지만 알려준다:
 *   - Refresh()        : 유닛 데이터가 확정됐다
 *   - NotifyHealthPresented() : 연출이 체력 변화를 표시했다
 *
 * 보스처럼 다른 규칙이 필요하면 UpdateVisibility / CalculateHeight를 오버라이드한다.
 */
	UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
	class SYNCHROPOST_API UUnitHealthBarComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UUnitHealthBarComponent();

	/** 유닛 데이터가 확정된 뒤 호출한다. 위젯 클래스 / 높이 / 초기값을 갱신하고 첫 표시 판정을 돌린다.
	 *  멱등이다 — OnRep과 BeginPlay 양쪽에서 불려 호출 순서에 무관하게 동작해야 한다. */
	void Refresh(AUnit* InOwnerUnit);

	/** 연출이 체력 변화를 표시한 시점에 호출한다.
	 *  게이지/숫자를 갱신하고, 연출이 끝날 때까지 바를 붙잡아 둔다. */
	void NotifyHealthPresented(int32 DisplayAmount, int32 NewTargetHealth,
		bool bIsCritical, const FGameplayTagContainer& TypeTags);

	void SetHovered(bool bHovered);
	void SetSelected(bool bSelected);

protected:

	virtual void BeginPlay() override;

	/** 정책 + 연출 체력 + 사건 이유를 종합해 표시 여부를 결정한다. */
	virtual void UpdateVisibility();

	/** 메시 기준 상대 Z 높이. */
	virtual float CalculateHeight() const;

	UUnitHealthBarWidget* GetBarWidget() const;
	AUnit* GetOwnerUnit() const { return CachedOwnerUnit.Get(); }
	const UUnitDataAsset* GetUnitData() const;

	/** 사건성 이유 하나를 켜거나 끈다. 실제로 바뀐 경우에만 표시를 갱신한다. */
	void SetReason(EHealthBarReason Reason, bool bEnable);

private:
	void ClearRecentHit();

	EHealthBarReason Reasons = EHealthBarReason::None;

	FTimerHandle RecentHitTimerHandle;

	TWeakObjectPtr<AUnit> CachedOwnerUnit;

	/** 피격 후 체력바를 붙잡아 둘 시간(초). 연출이 끝나기 전에 사라지지 않게 한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	float RecentHitHoldSeconds = 2.0f;

	/** 기준 높이를 못 구했을 때의 최후 기본값. */
	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	float FallbackHeight = 120.0f;
};
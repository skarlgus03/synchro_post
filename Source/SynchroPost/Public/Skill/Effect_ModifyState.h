#pragma once

#include "CoreMinimal.h"
#include "Skill/SkillEffect.h"
#include "Effect_ModifyState.generated.h"

UENUM(BlueprintType)
enum class EStateModifyMode : uint8
{
	Apply  UMETA(DisplayName = "걸기"),
	Remove UMETA(DisplayName = "제거"),
	Toggle UMETA(DisplayName = "토글 (있으면 제거, 없으면 걸기)"),
};

/**
 * 대상(유닛)의 상태를 걸거나, 제거하거나, 토글한다.
 * 상태이상 컴포넌트는 유닛만 가지므로 장애물 등은 건너뛴다.
 */
UCLASS(meta = (DisplayName = "상태 변경"))
class SYNCHROPOST_API UEffect_ModifyState : public USkillEffect
{
	GENERATED_BODY()

protected:
	virtual void ApplyTo(FSkillEffectContext& Ctx, const FAffectedTarget& Target) const override;

	UPROPERTY(EditAnywhere, Category = "State")
	EStateModifyMode Mode = EStateModifyMode::Apply;

	// 어떤 상태를, 몇 턴, 몇 % 확률로 (확률은 모든 모드에 적용)
	UPROPERTY(EditAnywhere, Category = "State")
	FSkillStatusEffectSpec Spec;
};
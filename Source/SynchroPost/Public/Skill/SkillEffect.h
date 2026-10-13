#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SPSkillStructure.h"
#include "Types/SynchroPostTypes.h"
#include "SkillEffect.generated.h"

class AUnit;
class USkillBase;
class USkillCondition;

UENUM(BlueprintType)
enum class ESkillEffectTarget : uint8
{
	AffectedTargets UMETA(DisplayName = "범위 안 대상들"),
	Caster UMETA(DisplayName = "시전자"),
};


/*
* Effect 실행에 필요한 정보 묶음.
* 실행기(USkillBase)가 스킬 1회마다 만들어 모든줄에 넘긴다.
* 서버에서만 만들어지고 실행이 끝나면 사라진다.
*/
struct FSkillEffectContext
{
	USkillBase* Skill = nullptr;
	AUnit* Caster = nullptr;
	FSkillExecutionContext ExecContext;

	// 타게팅 규칙으로 모은 범위 안 대상 들
	TArray<FAffectedTarget> AffectedTargets;

	// 연출이 읽을 결과
	TArray<FCombatEventTarget> Results;

	/** 범위 안 대상마다 빈 결과(체력 전후 = 현재, 양 0)를 만든다. 연출이 "누가 맞았나"를 알 수 있게 */
	void SeedResults();

	/** 이 대상의 결과를 찾고, 없으면 새로 만든다 (시전자 대상 Effect 등) */
	FCombatEventTarget& FindOrAddResult(const FAffectedTarget& Target);
};


/**
 * 스킬 효과의 한 줄: 언제(Conditions) / 누구에게(TargetType) / 무엇을(자식 클래스).
 *
 * DA에 들어 있는 레시피라 같은 스킬을 쓰는 모든 유닛이 공유한다.
 * 실행 중에 자기 멤버를 바꾸지 않는다 (전부 const).
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType)
class SYNCHROPOST_API USkillEffect : public UObject
{
	GENERATED_BODY()

public:
	/** 실행기가 부르는 입구. 대상을 정하고, 대상마다 조건을 확인한 뒤 ApplyTo를 부른다 */
	void Execute(FSkillEffectContext& Ctx) const;

protected:
	/**
	 * 조건을 통과한 대상 하나에 실제로 작용한다. 자식 클래스가 구현한다.
	 * Ctx.AffectedTargets는 순회 중이므로 바꾸지 말 것 (Results에만 쓴다).
	 */
	virtual void ApplyTo(FSkillEffectContext& Ctx, const FAffectedTarget& Target) const;

	/** 모든 조건을 만족하는가 (AND). 비어 있으면 통과 */
	bool PassesConditions(const FSkillEffectContext& Ctx, const AActor* Target) const;

	UPROPERTY(EditAnywhere, Category = "Effect")
	ESkillEffectTarget TargetType = ESkillEffectTarget::AffectedTargets;

	// 비어 있으면 항상 실행. 여러 개면 전부 만족해야 실행 (AND)
	UPROPERTY(EditAnywhere, Instanced, Category = "Effect")
	TArray<TObjectPtr<USkillCondition>> Conditions;
};
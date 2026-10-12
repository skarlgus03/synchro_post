#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Skill/SkillEffect.h"
#include "SkillCondition.generated.h"

/**
 * Effect 한 줄의 "언제". 대상마다 판단한다.
 * Target: 지금 판단 중인 대상 (시전자 줄이면 시전자).
 * 시전자를 봐야 하는 조건은 Target 대신 Ctx.Caster를 보면 된다.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType)
class SYNCHROPOST_API USkillCondition : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Check(const FSkillEffectContext& Ctx, const AActor* Target) const;
};
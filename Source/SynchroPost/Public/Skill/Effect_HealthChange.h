#pragma once

#include "CoreMinimal.h"
#include "Skill/SkillEffect.h"
#include "GameplayTagContainer.h"
#include "Effect_HealthChange.generated.h"

/**
 * 체력을 바꾸는 Effect의 공통 부분 : 시전자 스탯 x 계수 -> 대상에게 적용
 * 피해인지 회복인지는 자식이 붙이는 Action태그로 정해짐
 */
UCLASS(Abstract)
class SYNCHROPOST_API UEffect_HealthChange : public USkillEffect
{
	GENERATED_BODY()
	
protected:
	virtual void ApplyTo(FSkillEffectContext& Ctx, const FAffectedTarget& Target) const	override;

	// 이 Effect가 붙일 Action/Trait 태그. 자식이 정함.
	virtual FGameplayTagContainer BuildActionTags() const;

	// 시전자 스탯 태그 → 계수(%). 합산한 값이 양
	UPROPERTY(EditAnywhere, Category = "Amount", meta = (Categories = "Stat"))
	TMap<FGameplayTag, int32> Coefficients;

private:
	int32 CalculateAmount(AUnit* Caster) const;
};


UCLASS(meta = (DisplayName = "피해"))
class SYNCHROPOST_API UEffect_Damage : public UEffect_HealthChange
{
	GENERATED_BODY()

protected:
	virtual FGameplayTagContainer BuildActionTags() const override final;

	// 물리 / 마법 / 고정 중 하나. 방어 계산이 읽는다
	UPROPERTY(EditAnywhere, Category = "Damage", meta = (Categories = "Action.Damage"))
	FGameplayTag DamageType;

	// 피해 성질 (Trait.DamageType.* / Trait.Element.*). 저항 계산이 읽는다
	UPROPERTY(EditAnywhere, Category = "Damage", meta = (Categories = "Trait"))
	FGameplayTagContainer TraitTags;
};

UCLASS(meta = (DisplayName = "회복"))
class SYNCHROPOST_API UEffect_Heal : public UEffect_HealthChange
{
	GENERATED_BODY()

protected:
	virtual FGameplayTagContainer BuildActionTags() const override;
};
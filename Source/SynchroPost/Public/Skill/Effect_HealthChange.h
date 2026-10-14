#pragma once

#include "CoreMinimal.h"
#include "Skill/SkillEffect.h"
#include "GameplayTagContainer.h"
#include "Types/SynchroPostTypes.h"
#include "Effect_HealthChange.generated.h"

/** 피해 조각 하나의 설정: 어떤 방어로 깎이고(DamageType), 어떤 저항을 받고(Trait), 양은 시전자 스탯 x 계수 */
USTRUCT(BlueprintType)
struct FDamagePartSpec
{
	GENERATED_BODY()

	// 물리 / 마법 / 고정 중 하나. 방어 계산이 읽는다
	UPROPERTY(EditAnywhere, Category = "Damage", meta = (Categories = "Action.Damage"))
	FGameplayTag DamageType;

	// 피해 성질 (Trait.DamageType.* / Trait.Element.*). 저항 계산이 읽는다
	UPROPERTY(EditAnywhere, Category = "Damage", meta = (Categories = "Trait"))
	FGameplayTagContainer TraitTags;

	// 시전자 스탯 태그 → 계수(%). 합산한 값이 이 조각의 양
	UPROPERTY(EditAnywhere, Category = "Damage", meta = (Categories = "Stat"))
	TMap<FGameplayTag, int32> Coefficients;
};

/**
 * 체력을 바꾸는 Effect의 공통 흐름: 타격 하나 만들기(자식) → 대상에 적용 → 결과에 합치기 + 타격 기록.
 * Effect 한 줄 = 타격 한 번. 체력 변화와 반응(가시 등)은 타격당 한 번.
 */
UCLASS(Abstract)
class SYNCHROPOST_API UEffect_HealthChange : public USkillEffect
{
	GENERATED_BODY()
	
protected:
	virtual void ApplyTo(FSkillEffectContext& Ctx, const FAffectedTarget& Target) const	override;

	// 이 Effect의 타격 하나 (양·태그). 자식이 정함
	virtual FSPHealthActionData BuildHit(AUnit* Caster) const;

	// 시전자 스탯 x 계수 합
	static int32 CalculateAmount(AUnit* Caster, const TMap<FGameplayTag, int32>& Coefficients);
};


UCLASS(meta = (DisplayName = "피해"))
class SYNCHROPOST_API UEffect_Damage : public UEffect_HealthChange
{
	GENERATED_BODY()

protected:
	virtual FSPHealthActionData BuildHit(AUnit* Caster) const override final;

	// 한 타격 안의 피해 조각들. 조각마다 방어·저항을 따로 받고, 체력은 한 번에 깎인다
	UPROPERTY(EditAnywhere, Category = "Damage")
	TArray<FDamagePartSpec> Parts;
};

UCLASS(meta = (DisplayName = "회복"))
class SYNCHROPOST_API UEffect_Heal : public UEffect_HealthChange
{
	GENERATED_BODY()

protected:
	virtual FSPHealthActionData BuildHit(AUnit* Caster) const override final;

	// 시전자 스탯 태그 → 계수(%). 합산한 값이 회복량
	UPROPERTY(EditAnywhere, Category = "Amount", meta = (Categories = "Stat"))
	TMap<FGameplayTag, int32> Coefficients;
};
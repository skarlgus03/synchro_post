#include "Skill/Effect_HealthChange.h"
#include "Skill/SkillBase.h"
#include "Unit/Unit.h"
#include "Unit/StatComponent.h"
#include "Math/StatMath.h"
#include "Types/SPGameplayTags.h"
#include "SynchroPost.h"

// ── 공통 ─────────────────────────────────────────────────────

void UEffect_HealthChange::ApplyTo(FSkillEffectContext& Ctx, const FAffectedTarget& Target) const
{
	if (!Ctx.Skill)
	{
		UE_LOG(LogSP, Error, TEXT("[%s] 컨텍스트에 Skill 없음 → 건너뜀"), *GetClass()->GetName());
		return;
	}

	FSPHealthActionData ActionData;
	ActionData.Amount = CalculateAmount(Ctx.Caster);
	ActionData.ActionTypeTags = BuildActionTags();
	ActionData.Causer = Ctx.Caster;

	// 먼저 적용하고, 그 대상의 결과 하나에 합친다 (대상당 결과 하나 → 투사체·이펙트 중복 방지)
	const FCombatEventTarget Applied = Ctx.Skill->ApplyToTarget(Target, ActionData);

	FCombatEventTarget& Entry = Ctx.FindOrAddResult(Target);
	if (Entry.ActionData.Amount == 0)
	{
		Entry.ActionData = Applied.ActionData;               // 이 대상의 첫 체력 변화
	}
	else
	{
		Entry.ActionData.Amount += Applied.ActionData.Amount; // 여러 줄이면 양을 합친다
	}
	Entry.HealthAfterChange = Applied.HealthAfterChange;     // 체력 전은 시작 시점 값 유지
		
}

FGameplayTagContainer UEffect_HealthChange::BuildActionTags() const
{
	UE_LOG(LogSP, Error, TEXT("[%s] BuildActionTags가 구현되지 않음 → 태그 없음"), *GetClass()->GetName());
	return FGameplayTagContainer();
}

int32 UEffect_HealthChange::CalculateAmount(AUnit* Caster) const
{
	UStatComponent* CasterStat = Caster ? Caster->GetStatComponent() : nullptr;
	if (!CasterStat)
	{
		UE_LOG(LogSP, Warning, TEXT("[%s] 시전자 스탯 없음 → 양 0"), *GetClass()->GetName());
		return 0;
	}

	float Total = 0.f;
	for (const TPair<FGameplayTag, int32>& Pair : Coefficients)
	{
		Total += CasterStat->GetStat(Pair.Key) * StatMath::PercentToFloat(Pair.Value);
	}
	return FMath::RoundToInt(Total);
}

// ── 피해 ─────────────────────────────────────────────────────

FGameplayTagContainer UEffect_Damage::BuildActionTags() const
{
	FGameplayTagContainer Tags = TraitTags;
	if (DamageType.IsValid())
	{
		Tags.AddTag(DamageType);
	}
	else
	{
		UE_LOG(LogSP, Warning, TEXT("[Effect_Damage] 피해 유형이 비어 있음 → 방어 계산이 유형을 모름"));
	}
	return Tags;
}

// ── 회복 ─────────────────────────────────────────────────────

FGameplayTagContainer UEffect_Heal::BuildActionTags() const
{
	FGameplayTagContainer Tags;
	Tags.AddTag(SPTags::Action::Heal);
	return Tags;
}
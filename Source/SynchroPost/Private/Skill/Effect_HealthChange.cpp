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

	FSPHealthActionData Hit = BuildHit(Ctx.Caster);
	Hit.Causer = Ctx.Caster;

	// 타격 한 번 적용. 돌아오는 ActionData는 최종값 (조각별 방어·저항 반영)
	const FCombatEventTarget Applied = Ctx.Skill->ApplyToTarget(Target, Hit);

	// 대상당 결과 하나에 합친다 (투사체·이펙트 중복 방지)
	FCombatEventTarget& Entry = Ctx.FindOrAddResult(Target);
	if (Entry.Hits.Num() == 0)
	{
		Entry.ActionData = Applied.ActionData;   // 이 대상의 첫 타격
	}
	else
	{
		Entry.ActionData.Amount += Applied.ActionData.Amount;
		Entry.ActionData.ActionTypeTags.AppendTags(Applied.ActionData.ActionTypeTags);
	}
	Entry.HealthAfterChange = Applied.HealthAfterChange;   // 체력 전은 시작 시점 값 유지

	// 타격별 기록. 이미 죽었거나 체력이 없는 대상이라 적용되지 않았으면 남기지 않는다
	if (Applied.HealthBeforeChange != 0)
	{
		Entry.Hits.Add(Applied.ActionData);
	}
}

FSPHealthActionData UEffect_HealthChange::BuildHit(AUnit* Caster) const
{
	UE_LOG(LogSP, Error, TEXT("[%s] BuildHit이 구현되지 않음 → 빈 타격"), *GetClass()->GetName());
	return FSPHealthActionData();
}

int32 UEffect_HealthChange::CalculateAmount(AUnit* Caster, const TMap<FGameplayTag, int32>& Coefficients)
{
	UStatComponent* CasterStat = Caster ? Caster->GetStatComponent() : nullptr;
	if (!CasterStat)
	{
		UE_LOG(LogSP, Warning, TEXT("[Effect_HealthChange] 시전자 스탯 없음 → 양 0"));
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

FSPHealthActionData UEffect_Damage::BuildHit(AUnit* Caster) const
{
	FSPHealthActionData Hit;

	if (Parts.Num() == 0)
	{
		UE_LOG(LogSP, Warning, TEXT("[Effect_Damage] 피해 조각이 비어 있음 → 피해 0"));
	}

	for (const FDamagePartSpec& Spec : Parts)
	{
		FDamagePart& Part = Hit.Parts.AddDefaulted_GetRef();
		Part.Amount = CalculateAmount(Caster, Spec.Coefficients);
		Part.Tags = Spec.TraitTags;

		if (Spec.DamageType.IsValid())
		{
			Part.Tags.AddTag(Spec.DamageType);
		}
		else
		{
			UE_LOG(LogSP, Warning, TEXT("[Effect_Damage] 피해 유형이 빈 조각 → 방어 무시(고정 피해)로 계산됨"));
		}
	}

	Hit.SyncTotalsFromParts();   // 방어 전 합계·태그. ActionTypeTags를 보는 곳(회복 판정 등)용
	return Hit;
}

// ── 회복 ─────────────────────────────────────────────────────

FSPHealthActionData UEffect_Heal::BuildHit(AUnit* Caster) const
{
	FSPHealthActionData Hit;
	Hit.Amount = CalculateAmount(Caster, Coefficients);
	Hit.ActionTypeTags.AddTag(SPTags::Action::Heal);
	return Hit;
}
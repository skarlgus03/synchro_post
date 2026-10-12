#include "Skill/Effect_ModifyState.h"
#include "StatusEffect/StatusEffectDataAsset.h"
#include "Unit/Unit.h"
#include "Unit/StateComponent.h"
#include "SynchroPost.h"

void UEffect_ModifyState::ApplyTo(FSkillEffectContext& Ctx, const FAffectedTarget& Target) const
{
	if (!Spec.EffectData)
	{
		UE_LOG(LogSP, Error, TEXT("[Effect_ModifyState] 상태 DA가 비어 있음 → 건너뜀"));
		return;
	}

	AUnit* Unit = Cast<AUnit>(Target.Actor.Get());
	if (!Unit || Unit->IsDead())
	{
		return; // 유닛이 아니거나 이미 죽음 — 할 일이 없는 게 정상
	}

	UStateComponent* StateComp = Unit->GetStateComponent();
	if (!StateComp)
	{
		return;
	}

	if (FMath::RandHelper(100) >= Spec.ChancePercent)
	{
		return; // 확률 실패
	}

	const FGameplayTag StateTag = Spec.EffectData->StatusEffectTag;
	// 토글 판단은 정확 일치 (계층 매칭이면 하위 태그 상태에 반응할 수 있음)
	const bool bHasState = StateComp->GetStatusEffectCount(StateTag) > 0;

	// 상태 변경 모드에 따라 적용
	switch (Mode)
	{
	case EStateModifyMode::Apply:
		StateComp->ApplyStatusEffect(Spec.EffectData, Spec.Duration, Ctx.Caster);
		break;

	case EStateModifyMode::Remove:
		StateComp->RemoveFirstEffectByTag(StateTag);
		break;

	case EStateModifyMode::Toggle:
		if (bHasState)
		{
			StateComp->RemoveFirstEffectByTag(StateTag);
		}
		else
		{
			StateComp->ApplyStatusEffect(Spec.EffectData, Spec.Duration, Ctx.Caster);
		}
		break;
	}
}
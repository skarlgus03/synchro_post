#include "Skill/SkillStep.h"
#include "Unit/Unit.h"
#include "Interface/Damageable.h"

void USkillStep::Start(const FSkillPresentationContext& InCtx)
{
	Ctx = InCtx;
	Finish();
}

void USkillStep::Finish()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	OnStepFinished.ExecuteIfBound();
}

UWorld* USkillStep::GetContextWorld() const
{
	const AUnit* Caster = Ctx.Caster.Get();
	return Caster ? Caster->GetWorld() : nullptr;
}

void USkillStep::PresentTargetResult(const FCombatEventTarget& TargetData) const
{
	AActor* TargetActor = TargetData.Target.Get();
	if (!TargetActor || !TargetActor->Implements<UDamageable>())
	{
		return;
	}

	const int32 DisplayAmount = TargetData.HealthAfterChange - TargetData.HealthBeforeChange;
	UE_LOG(LogTemp, Warning, TEXT("[SP] 결과표시 %s : HP %d -> %d (%d)"),
		*GetNameSafe(TargetActor),
		TargetData.HealthBeforeChange, TargetData.HealthAfterChange, DisplayAmount);

	IDamageable::Execute_ApplyVisualDamage(
		TargetActor,
		DisplayAmount,
		TargetData.HealthAfterChange,
		TargetData.ActionData.bIsCriticalHit,
		TargetData.ActionData.ActionTypeTags);

}

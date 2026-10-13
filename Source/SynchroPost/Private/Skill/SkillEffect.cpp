#include "Skill/SkillEffect.h"
#include "Skill/SkillCondition.h"
#include "Unit/Unit.h"
#include "Interface/Damageable.h"
#include "SynchroPost.h"

void USkillEffect::Execute(FSkillEffectContext& Ctx) const
{
	if (TargetType == ESkillEffectTarget::Caster)
	{
		FAffectedTarget Self;
		Self.Actor = Ctx.Caster;
		Self.Coordinate = Ctx.ExecContext.CasterCoordinate;

		if (PassesConditions(Ctx, Self.Actor))
		{
			ApplyTo(Ctx, Self);
		}
		return;
	}
	for (const FAffectedTarget& Target : Ctx.AffectedTargets)
	{
		if (PassesConditions(Ctx, Target.Actor.Get()))
		{
			ApplyTo(Ctx, Target);
		}
	}
}

void USkillEffect::ApplyTo(FSkillEffectContext& Ctx, const FAffectedTarget& Target) const
{
	UE_LOG(LogSP, Error, TEXT("[%s] ApplyTo not implemented!"), *GetName());
}

bool USkillEffect::PassesConditions(const FSkillEffectContext& Ctx, const AActor* Target) const
{
	for (const USkillCondition* Condition : Conditions)
	{
		if (!Condition)
		{
			UE_LOG(LogSP, Warning, TEXT("[Effect] %s : Condition is null!"), *GetName());
			continue;
		}
		if (!Condition->Check(Ctx, Target))
		{
			return false;
		}
	}
	return true;
}


static FCombatEventTarget MakeEmptyResult(const FAffectedTarget& Target)
{
	FCombatEventTarget Result;
	Result.Coordinate = Target.Coordinate;

	AActor* Actor = Target.Actor.Get();
	if (Actor && Actor->Implements<UDamageable>())
	{
		Result.Target = Actor;
		Result.HealthBeforeChange = IDamageable::Execute_GetCurrentHealth(Actor);
		Result.HealthAfterChange = Result.HealthBeforeChange;
	}
	return Result;
}

void FSkillEffectContext::SeedResults()
{
	Results.Reset();
	for (const FAffectedTarget& Target : AffectedTargets)
	{
		Results.Add(MakeEmptyResult(Target));
	}
}

FCombatEventTarget& FSkillEffectContext::FindOrAddResult(const FAffectedTarget& Target)
{
	for (FCombatEventTarget& Result : Results)
	{
		if (Result.Coordinate == Target.Coordinate && Result.Target.Get() == Target.Actor.Get())
		{
			return Result;
		}
	}
	return Results.Add_GetRef(MakeEmptyResult(Target));
}
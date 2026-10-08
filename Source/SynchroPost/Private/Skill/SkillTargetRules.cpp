#include "Skill/SkillTargetRules.h"
#include "Unit/Unit.h"


namespace SkillTargetRules
{
	ETargetRelation GetRelation(const AActor* Occupant, const AActor* Caster, EFaction CasterFaction)
	{
		const AUnit* Unit = Cast<AUnit>(Occupant);
		if (!Unit || Unit->IsDead())
		{
			return ETargetRelation::None;
		}
		if (Unit == Caster)
		{
			return ETargetRelation::Self;
		}
		return Unit->GetFaction() == CasterFaction ? ETargetRelation::Ally : ETargetRelation::Enemy;
	}

	bool Allows(ESkillTargetFaction Rule, ETargetRelation Relation)
	{
		switch (Rule)
		{
		case ESkillTargetFaction::Any:
			return Relation != ETargetRelation::None;
		case ESkillTargetFaction::Ally:
			return Relation == ETargetRelation::Self || Relation == ETargetRelation::Ally;   // 기존: 같은 진영 = 자신 포함
		case ESkillTargetFaction::Enemy:
			return Relation == ETargetRelation::Enemy;
		case ESkillTargetFaction::None:
		default:
			return false;
		}
	}
}

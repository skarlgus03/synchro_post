#include "Skill/SkillTargetRules.h"
#include "Unit/Unit.h"
#include "Grid/GridObstacle.h"


namespace SkillTargetRules
{
	ETargetRelation GetRelation(const AActor* Occupant, const AActor* Caster, EFaction CasterFaction)
	{
		if (const AGridObstacle* Obstacle = Cast<AGridObstacle>(Occupant))
		{
			return Obstacle->IsDestroyed() ? ETargetRelation::None : ETargetRelation::Obstacle;
		}

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

	ESkillTargetFlags ToFlag(ETargetRelation Relation)
	{
		switch (Relation)
		{
		case ETargetRelation::Self:     return ESkillTargetFlags::Self;
		case ETargetRelation::Ally:     return ESkillTargetFlags::Ally;
		case ETargetRelation::Enemy:    return ESkillTargetFlags::Enemy;
		case ETargetRelation::Obstacle: return ESkillTargetFlags::Obstacle;
		case ETargetRelation::None:
		default:                        return ESkillTargetFlags::EmptyTile;
		}
	}

	bool Allows(int32 Mask, ETargetRelation Relation)
	{
		return (Mask & static_cast<int32>(ToFlag(Relation))) != 0;
	}
}

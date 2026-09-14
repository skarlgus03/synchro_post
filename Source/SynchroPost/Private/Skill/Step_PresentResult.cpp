
#include "Skill/Step_PresentResult.h"
#include "Interface/Damageable.h"
#include "Types/SynchroPostTypes.h"

void UStep_PresentResult::Start(const FSkillPresentationContext& InCtx)
{
	Ctx = InCtx;

	for (const FCombatEventTarget& TargetData : Ctx.Payload.Targets)
	{
		PresentTargetResult(TargetData);
	}

	Finish();
}

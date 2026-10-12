#include "Skill/SkillCondition.h"
#include "SynchroPost.h"

bool USkillCondition::Check(const FSkillEffectContext& Ctx, const AActor* Target) const
{
	UE_LOG(LogSP, Error, TEXT("[Condition] %s : Check가 구현되지 않음. 실패"), *GetClass()->GetName());
	return false;
}

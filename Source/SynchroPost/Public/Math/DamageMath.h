#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "Types/SynchroPostTypes.h"

namespace DamageMath
{
	using FDefenseFn = TFunctionRef<float(const FGameplayTagContainer& /*PartTags*/, const FPenetrationData&)>;
	using FResistanceFn = TFunctionRef<float(const FGameplayTagContainer& /*PartTags*/)>;

	/**
	 * 피해 조각마다 방어·저항 배율을 곱해 최종 양을 채운다. 최소 피해 1은 타격 전체에 한 번.
	 * 끝나면 Amount = 합, ActionTypeTags = 합집합. 체력은 건드리지 않는다 (저장 위치는 호출자마다 다름).
	 * 반환: 최종 피해 합
	 */
	SYNCHROPOST_API int32 ResolveDamage(FSPHealthActionData& Data, FDefenseFn DefenseMultiplier, FResistanceFn ResistanceMultiplier);
}
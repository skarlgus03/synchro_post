#pragma once

#include "CoreMinimal.h"
#include "Types/SynchroPostTypes.h"
#include "Types/SPSkillStructure.h"

class AActor;

// 칸 위의 액터가 시전자에게 무엇인가
enum class ETargetRelation : uint8
{
	None,
	Self,
	Ally,
	Enemy,
};


/*
* 스킬 대상 판정의 단일 출처. 진영 비교는 이 파일에만 있다.
* 선택 검증,대상 수집,AI가 전부 여기를 거쳐서 판정하도록 한다.
*/
namespace SkillTargetRules
{
	/*중립은 지금 Enemy로 취급함.*/
	SYNCHROPOST_API ETargetRelation GetRelation(const AActor* Occupant, const AActor* Caster, EFaction CasterFaction);

	/*이 규칙이 그 관계를 허용하는가. None 규칙은 호출부가 따로 처리함.*/
	SYNCHROPOST_API bool Allows(ESkillTargetFaction Rule, ETargetRelation Relation);
}
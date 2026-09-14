#include "Animation/AnimNotify_SkillCue.h"
#include "Unit/Unit.h"
#include "Unit/SkillComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_SkillCue::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase*,
	const FAnimNotifyEventReference&)
{
	if (!MeshComp) { return; }

	// 에디터 프리뷰(Persona)에서는 Owner가 AUnit이 아니라 그냥 빠져나간다.
	AUnit* Unit = Cast<AUnit>(MeshComp->GetOwner());
	if (!Unit) { return; }

	if (USkillComponent* SkillComp = Unit->GetSkillComponent())
	{
		SkillComp->BroadcastSkillCue(CueTag);
	}
}

FString UAnimNotify_SkillCue::GetNotifyName_Implementation() const
{
	return CueTag.IsValid() ? CueTag.GetTagName().ToString() : TEXT("SkillCue (미지정)");
}
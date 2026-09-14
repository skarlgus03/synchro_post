#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_SkillCue.generated.h"

/**
 * "이 프레임에 무슨 일이 일어날 시점이다"만 알린다.
 * 무엇을 스폰할지, 얼마나 때릴지는 전혀 모른다.
 */
UCLASS(DisplayName = "Skill Cue")
class SYNCHROPOST_API UAnimNotify_SkillCue : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Skill", meta = (Categories = "Skill.Cue"))
	FGameplayTag CueTag;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// 몽타주 타임라인에 태그 이름이 보이게
	virtual FString GetNotifyName_Implementation() const override;
};
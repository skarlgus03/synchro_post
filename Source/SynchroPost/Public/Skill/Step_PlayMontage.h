
#pragma once

#include "CoreMinimal.h"
#include "Skill/SkillStep.h"
#include "Step_PlayMontage.generated.h"

class UAnimMontage;
class UAnimInstance;
class SkillComponent;

UENUM(BlueprintType)
enum class ESkillStepMontageEnd : uint8
{
	// 몽타주가 끝날 때까지 기다린다.
	MontageEnded UMETA(DisplayName = "몽타주 종료까지 대기"),

	// 재생만 걸고 바로 다음 줄로 넘어간다. (몽타주는 끊기지 않고 계속 재생된다.)
	Immediate UMETA(DisplayName = "즉시 종료"),

	// 노티파이 큐를 받으면 다음 줄로 넘어간다. (몽타주는 끊기지 않고 계속 재생된다.)
	CueReceived UMETA(DisplayName = "큐를 받으면 다음 줄로"),
};

/*
* 몽타주를 재생한다.
*/
UCLASS(DisplayName = "몽타주 재생")
class SYNCHROPOST_API UStep_PlayMontage : public USkillStep
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Montage")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, Category = "Montage")
	FName StartSection = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Montage", meta = (ClampMin = "0.01"))
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, Category = "Montage")
	ESkillStepMontageEnd EndCondition = ESkillStepMontageEnd::MontageEnded;

	UPROPERTY(EditAnywhere, Category = "Montage",
		meta = (EditCondition = "EndCondition == ESkillStepMontageEnd::CueReceived",
			EditConditionHides, Categories = "Skill.Cue"))
	FGameplayTag WaitCueTag;

	virtual void Start(const FSkillPresentationContext& InCtx) override;
	virtual void Abort() override;

private:
	UFUNCTION()
	void HandleMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted);

	void HandleSkillCue(FGameplayTag CueTag);
	void UnbindAll();

	FDelegateHandle CueHandle;
	TWeakObjectPtr<USkillComponent> BoundSkillComp;

	TWeakObjectPtr<UAnimInstance> BoundAnimInstance;

};

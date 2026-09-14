#pragma once

#include "CoreMinimal.h"
#include "Skill/SkillStep.h"
#include "Step_Wait.generated.h"

/** N초 기다린다. 연출 템포 조절용. */
UCLASS(DisplayName = "대기")
class SYNCHROPOST_API UStep_Wait : public USkillStep
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "대기", meta = (ClampMin = "0.0"))
	float Seconds = 0.5f;

	virtual void Start(const FSkillPresentationContext& InCtx) override;
	virtual void Abort() override;

private:
	FTimerHandle TimerHandle;
};
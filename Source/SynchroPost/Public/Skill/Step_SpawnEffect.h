#pragma once

#include "CoreMinimal.h"
#include "Skill/SkillStep.h"
#include "Step_SpawnEffect.generated.h"

class UNiagaraSystem;

/*
* 타겟 위치에 일회성 나이아가라 이펙트를 생성한다.
*/
UCLASS(DisplayName = "이펙트 생성")
class SYNCHROPOST_API UStep_SpawnEffect : public USkillStep
{
	GENERATED_BODY()
	

public:

	UPROPERTY(EditAnywhere, Category = "Effect")
	TSoftObjectPtr<UNiagaraSystem> Effect;

	// 타겟 액터의 소켓. 비었거나 존재하지 않으면 액터의 위치를 사용한다.
	UPROPERTY(EditAnywhere, Category = "Effect")
	FName SocketName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Effect")
	FVector Offset = FVector::ZeroVector;

	// true이면 타겟마다 하나씩, false이면 타겟들의 중심에 하나만 사용한다.
	UPROPERTY(EditAnywhere, Category = "Effect")
	bool bPerTarget = true;

	// 만약 0이면 스폰하고 즉시 다음 줄로 넘어간다. 0보다 크면 WaitSeconds만큼 대기 후 Finish()를 호출한다.
	UPROPERTY(EditAnywhere, Category = "Effect", meta = (ClampMin = "0.0"))
	float WaitSeconds = 0.0f;

	virtual void Start(const FSkillPresentationContext& InCtx) override;
	virtual void Abort() override;

private:
	void HandleWaitFinished() { Finish(); }

	FTimerHandle WaitTimer;
};

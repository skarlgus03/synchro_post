#pragma once

#include "CoreMinimal.h"
#include "Skill/SkillStep.h"
#include "Step_PresentResult.generated.h"

/**
 * 서버가 이미 계산해둔 결과(Payload.Targets)를 화면에 표시한다.
 * 체력바 갱신 / 데미지 숫자 / 피격 이펙트가 여기서 발생한다.
 *
 * 표시만 하고 즉시 종료한다. 숫자를 읽을 시간이 필요하면
 * 대본에서 이 줄 뒤에 "대기" 줄을 붙일 것.
 */
UCLASS(DisplayName = "결과 표시")
class SYNCHROPOST_API UStep_PresentResult : public USkillStep
{
	GENERATED_BODY()

public:
	virtual void Start(const FSkillPresentationContext& InCtx) override;
};
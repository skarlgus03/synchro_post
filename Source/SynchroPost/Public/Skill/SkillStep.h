#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SPCombatEventStructure.h"
#include "SkillStep.generated.h"

class AUnit;
class USkillComponent;

/*
* 연출 스텝이 실행될 때 필요한 정보 묶음.
* Presentation이 만들어서 모든 Step 에 그대로 넘긴다
*/

USTRUCT(BlueprintType)
struct FSkillPresentationContext
{
	GENERATED_BODY()

	// 스킬 시전자
	UPROPERTY(BlueprintReadOnly, Category = "Presentation")
	TWeakObjectPtr<AUnit> Caster;

	// AnimNotify 큐 라우팅 용
	UPROPERTY(BlueprintReadOnly, Category = "Presentation")
	TWeakObjectPtr<USkillComponent> OwnerComp;

	// 서버가 이미 계산한 결과. 읽기 전용으로 사용. (타겟 좌표, 피해량 등)
	UPROPERTY(BlueprintReadOnly, Category = "Presentation")
	FSkillEventPayload Payload;
};

/**
 * 연출 대본의 한 줄.
 *
 * 책임은 두 가지뿐이다.
 *   (1) 뭔가를 시작한다
 *   (2) 그게 끝나면 Finish()를 부른다
 * 판단이나 계산은 하지 않는다.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType)
class SYNCHROPOST_API USkillStep : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE(FOnStepFinished);
	FOnStepFinished OnStepFinished;

	/**
	 * 스텝 시작. 자식 클래스가 오버라이드한다.
	 * 기본 구현이 '즉시 종료'인 이유 - 잘못 만든 스텝 하나가 연출 큐 전체를
	 * 멈추게 하면 턴제에서는 게임이 그대로 정지한다. PURE_VIRTUAL을 쓰지 않는다.
	 */
	virtual void Start(const FSkillPresentationContext& InCtx);

	/** 연출 중단(스킵 / 타임아웃) 시 정리 */
	virtual void Abort() {}

protected:
	/** "나 끝났어". 두 번 불려도 한 번만 통과시킨다. */
	void Finish();

	/** Caster를 통해 월드를 얻는다. UObject라 GetWorld()를 믿을 수 없다. */
	UWorld* GetContextWorld() const;

	UPROPERTY()
	FSkillPresentationContext Ctx;

	bool IsFinished() const { return bFinished; }

	void PresentTargetResult(const FCombatEventTarget& TargetData) const;
private:
	bool bFinished = false;
};
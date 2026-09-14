#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Skill/SkillStep.h"
#include "SkillPresentation.generated.h"

/**
 * 스킬 연출 대본.
 * Step 배열을 위에서부터 하나씩 실행하고, 전부 끝나면 OnFinished를 알린다.
 * 로직 판단은 전혀 하지 않는다 - '순서'만 책임진다.
 *
 * DA에 붙어있는 인스턴스는 '템플릿'이다. 실행할 때 반드시 복제해서 쓸 것.
 */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType)
class SYNCHROPOST_API USkillPresentation : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE(FOnPresentationFinished);
	FOnPresentationFinished OnFinished;

	UPROPERTY(EditAnywhere, Instanced, Category = "연출")
	TArray<TObjectPtr<USkillStep>> Steps;
	// 연출 시작. Ctx를 복사하고, Step 배열을 위에서부터 순차적으로 실행한다.
	void Play(const FSkillPresentationContext& InCtx);
	// 연출 중단(스킵 / 타임아웃) 시 정리. 현재 스텝의 Abort()를 호출한다.
	void Abort();

private:
	void Advance();
	void FinishPresentation();

	UPROPERTY()
	FSkillPresentationContext Ctx;

	int32 CurrentIndex = INDEX_NONE;

	// Step이 Start() 안에서 즉시 Finish()를 부르면 Advance()가 재귀한다.
	// 스텝이 많으면 스택이 깊어지므로 루프로 평탄화하기 위한 플래그.
	bool bIsAdvancing = false;
	bool bAdvanceRequested = false;
	bool bFinished = false;
};
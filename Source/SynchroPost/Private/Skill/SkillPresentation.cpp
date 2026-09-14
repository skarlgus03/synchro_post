#include "Skill/SkillPresentation.h"

void USkillPresentation::Play(const FSkillPresentationContext& InCtx)
{
	UE_LOG(LogTemp, Warning, TEXT("[SP] 2. Presentation::Play (Steps=%d)"), Steps.Num());


	Ctx = InCtx;
	CurrentIndex = INDEX_NONE;
	bFinished = false;
	bIsAdvancing = false;
	bAdvanceRequested = false;

	Advance();
}

void USkillPresentation::Advance()
{
	// 이미 Advance 루프 안이면 재귀하지 말고 루프에게 맡긴다.
	if (bIsAdvancing)
	{
		bAdvanceRequested = true;
		return;
	}

	bIsAdvancing = true;

	do
	{
		bAdvanceRequested = false;

		// 직전 스텝의 바인딩을 해제
		if (Steps.IsValidIndex(CurrentIndex) && Steps[CurrentIndex])
		{
			Steps[CurrentIndex]->OnStepFinished.Unbind();
		}

		// 인덱스를 1더하고 다음 유효 스텝 찾기 (에디터에서 비워둔 슬롯은 건너뜀)
		do
		{
			++CurrentIndex;
		} while (Steps.IsValidIndex(CurrentIndex) && !Steps[CurrentIndex]);

		// 대본 배열을 전부 소진했으면, 연출을 종료함.
		if (!Steps.IsValidIndex(CurrentIndex))
		{
			bIsAdvancing = false;
			FinishPresentation();
			return;
		}

		USkillStep* Step = Steps[CurrentIndex];
		Step->OnStepFinished.BindUObject(this, &USkillPresentation::Advance);

		UE_LOG(LogTemp, Warning, TEXT("[SP] 3. Step[%d] 시작: %s"), CurrentIndex, *GetNameSafe(Step));
		Step->Start(Ctx);
	} while (bAdvanceRequested);

	bIsAdvancing = false;
}

void USkillPresentation::Abort()
{
	// 현재 스탭이 있으면 Abort()를 호출하고 바인딩 해제
	if (Steps.IsValidIndex(CurrentIndex) && Steps[CurrentIndex])
	{
		Steps[CurrentIndex]->OnStepFinished.Unbind();
		Steps[CurrentIndex]->Abort();
	}
	// 연출을 종료함. CurrentIndex를 Steps.Num()으로 설정하면 Advance()가 더 이상 진행되지 않는다.
	CurrentIndex = Steps.Num();
	FinishPresentation();
}

void USkillPresentation::FinishPresentation()
{
	if (bFinished)
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("[SP] 4. Presentation 종료"));

	bFinished = true;
	OnFinished.ExecuteIfBound();
}
#include "Skill/Step_Wait.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UStep_Wait::Start(const FSkillPresentationContext& InCtx)
{
	Ctx = InCtx;

	UWorld* World = GetContextWorld();
	if (!World || Seconds <= 0.f)
	{
		Finish();
		return;
	}

	World->GetTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateUObject(this, &UStep_Wait::Finish),
		Seconds, false);
}

void UStep_Wait::Abort()
{
	if (UWorld* World = GetContextWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
}
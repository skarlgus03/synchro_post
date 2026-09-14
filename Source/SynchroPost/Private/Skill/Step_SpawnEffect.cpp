#include "Skill/Step_SpawnEffect.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"

void UStep_SpawnEffect::Start(const FSkillPresentationContext& InCtx)
{
	Ctx = InCtx;

	UWorld* World = GetContextWorld();
	UNiagaraSystem* System = Effect.LoadSynchronous();

	if (!World || !System || Ctx.Payload.Targets.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SP] 이펙트 스폰: 이펙트 또는 타겟 없음"));
		Finish();
		return;
	}

	if (bPerTarget)
	{
		for (const FCombatEventTarget& TargetData : Ctx.Payload.Targets)
		{
			const FVector Location = ResolveTargetLocation(TargetData, SocketName, Offset);
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Location);
		}
		UE_LOG(LogTemp, Warning, TEXT("[SP] 이펙트 %d개 스폰"), Ctx.Payload.Targets.Num());
	}
	else
	{
		// 타겟들의 중심 한 곳에만. Offset은 중심을 구한 뒤 한 번만 더한다.
		FVector Center = FVector::ZeroVector;
		for (const FCombatEventTarget& TargetData : Ctx.Payload.Targets)
		{
			Center += ResolveTargetLocation(TargetData, SocketName);
		}
		Center /= static_cast<float>(Ctx.Payload.Targets.Num());

		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Center + Offset);
		UE_LOG(LogTemp, Warning, TEXT("[SP] 이펙트 1개 스폰 (타겟 중심)"));
	}

	if (WaitSeconds <= 0.f)
	{
		Finish();
		return;
	}

	World->GetTimerManager().SetTimer(WaitTimer, this,
		&UStep_SpawnEffect::HandleWaitFinished, WaitSeconds, false);
}

void UStep_SpawnEffect::Abort()
{
	if (UWorld* World = GetContextWorld())
	{
		World->GetTimerManager().ClearTimer(WaitTimer);
	}
}

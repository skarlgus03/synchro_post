#include "Framework/CombatEventComponent.h"
#include "Unit/Unit.h"
#include "Skill/SkillBase.h"
#include "Unit/SkillComponent.h"

UCombatEventComponent::UCombatEventComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCombatEventComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UCombatEventComponent::PushEvent(const FCombatEvent& NewEvent)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	MulticastPresentCombatEvent(NewEvent);
}

void UCombatEventComponent::MulticastPresentCombatEvent_Implementation(const FCombatEvent& Event)
{
	LocalPresentationQueue.Add(Event);

	if (!bIsProcessing)
	{
		ProcessNextQueuedEvent();
	}
}

void UCombatEventComponent::ProcessNextQueuedEvent()
{
	if (LocalPresentationQueue.Num() == 0)
	{
		bIsProcessing = false;
		return;
	}

	bIsProcessing = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PresentationTimeoutHandle, this,
			&UCombatEventComponent::ForceFinishPresentation,
			PresentationTimeoutSeconds, false);
	}

	FCombatEvent Event = LocalPresentationQueue[0];
	LocalPresentationQueue.RemoveAt(0);

	// 연출 분기

	if (const FSkillEventPayload* SkillPayload = Event.Payload.GetPtr<FSkillEventPayload>())
	{
		AUnit* Caster = Event.Source.Get();
		USkillBase* Skill = (Caster && Caster->GetSkillComponent())
			? Caster->GetSkillComponent()->FindSkillByTag(SkillPayload->SkillTag)
			: nullptr;

		if (Skill)
		{
			Skill->PresentSkillEffect(Event);
		}
		else
		{
			NotifyPresentationFinished();
		}
	}
	else if (const FUnitDiedPayload* DiedPayload = Event.Payload.GetPtr<FUnitDiedPayload>())
	{
		if (AUnit* DiedUnit = Event.Source.Get())
		{
			DiedUnit->PresentDeath();
		}
		else
		{
			NotifyPresentationFinished();
		}
	}
	else if (const FUnitRevivedPayload* RevivedPayload = Event.Payload.GetPtr<FUnitRevivedPayload>())
	{
		if (AUnit* RevivedUnit = Event.Source.Get())
		{
			RevivedUnit->PresentRevive();
		}
		else
		{
			NotifyPresentationFinished();
		}
	}
	else if (const FMoveEventPayload* MovePayload = Event.Payload.GetPtr<FMoveEventPayload>())
	{
		if (AUnit* Target = Event.Source.Get())
		{
			Target->PresentMoveSegment(MovePayload->From, MovePayload->To);
		}
		else
		{
			NotifyPresentationFinished();
		}
	}
	else if (const FTriggerEventPayload* TriggerPayload = Event.Payload.GetPtr<FTriggerEventPayload>())
	{
		if (TriggerPayload->Trigger.GetObject())
		{
			ITileTrigger::Execute_PresentTriggerEffect(TriggerPayload->Trigger.GetObject(), TriggerPayload->Result);
		}
		else
		{
			NotifyPresentationFinished();
		}
	}
	else
	{
		// 알수 없는 이벤트 타입, 그냥 다음 이벤트로 넘어감
		NotifyPresentationFinished();
	}
}

void UCombatEventComponent::ForceFinishPresentation()
{
	UE_LOG(LogTemp, Warning, TEXT("[CombatEvent] 연출 타임아웃 - 강제 진행"));
	NotifyPresentationFinished();
}

void UCombatEventComponent::NotifyPresentationFinished()
{
	// 타임아웃이 먼저 터진 뒤 늦게 도착한 완료 신호를 무시한다.
	// (없으면 이벤트 하나가 통째로 건너뛰어진다)
	if (!bIsProcessing)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PresentationTimeoutHandle);
	}
	ProcessNextQueuedEvent();
}
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

void UCombatEventComponent::NotifyPresentationFinished()
{
	ProcessNextQueuedEvent();
}
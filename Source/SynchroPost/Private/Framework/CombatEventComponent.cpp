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

	
	switch (Event.EventType)
	{
	case ECombatEventType::SkillUsed:
	{
		AUnit* Caster = Event.Source.Get();
		USkillBase* Skill = (Caster && Caster->GetSkillComponent())
			? Caster->GetSkillComponent()->FindSkillByTag(Event.SkillTag)
			: nullptr;

		if (Skill)
		{
			Skill->PresentSkillEffect(Event);
		}
		else
		{
			NotifyPresentationFinished();   // 못 찾으면 그냥 건너뜀
		}
		break;
	}
	case ECombatEventType::UnitDied:
	{
		AUnit* Target = (Event.Targets.Num() > 0) ? Event.Targets[0].Target.Get() : nullptr;
		if (Target)
		{
			Target->PresentDeath();
		}
		else
		{
			NotifyPresentationFinished();
		}
		break;
	}
	case ECombatEventType::UnitRevived:
	{
		AUnit* Target = (Event.Targets.Num() > 0) ? Event.Targets[0].Target.Get() : nullptr;
		if (Target)
		{
			Target->PresentRevive();
		}
		else
		{
			NotifyPresentationFinished();
		}
		break;
	}
	}
}

void UCombatEventComponent::NotifyPresentationFinished()
{
	ProcessNextQueuedEvent();
}
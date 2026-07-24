#include "Framework/CombatEventComponent.h"
#include "Unit/Unit.h"

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

	// TODO: 실제 연출 위임
	// - Event.SkillTag가 있으면: Event.Source->GetSkillComponent()->FindSkillByTag(...)로 스킬 찾아서 연출 호출
	// - EventType이 UnitDied/UnitRevived면: 해당 Unit한테 직접 연출 함수 호출
	// - 연출 끝나면 그쪽에서 NotifyPresentationFinished() 다시 불러줘야 큐가 계속 진행됨
}

void UCombatEventComponent::NotifyPresentationFinished()
{
	ProcessNextQueuedEvent();
}
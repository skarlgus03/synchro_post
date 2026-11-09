#include "Framework/CombatEventComponent.h"
#include "Unit/Unit.h"
#include "Skill/SkillBase.h"
#include "Unit/SkillComponent.h"
#include "SynchroPost.h"
#include "Framework/RunProgressComponent.h"

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
		SetProcessing(false);
		return;
	}

	SetProcessing(true);
	PendingFinishCount = 0;

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
	UE_LOG(LogTemp, Verbose, TEXT("[Q] 처리: %s | Source=%s | 남은 %d개"),
		*GetNameSafe(Event.Payload.GetScriptStruct()),
		*GetNameSafe(Event.Source.Get()),
		LocalPresentationQueue.Num());

	// 이벤트 종류를 고르기만 한다. 보여주는 방법은 각 Present 함수가 맡는다
	if (const FSkillEventPayload* SkillPayload = Event.Payload.GetPtr<FSkillEventPayload>())
	{
		PresentSkill(Event, *SkillPayload);
	}
	else if (const FUnitsDiedPayload* DeathsPayload = Event.Payload.GetPtr<FUnitsDiedPayload>())
	{
		PresentDeaths(*DeathsPayload);
	}
	else if (Event.Payload.GetPtr<FUnitRevivedPayload>())
	{
		PresentRevive(Event);
	}
	else if (const FMoveEventPayload* MovePayload = Event.Payload.GetPtr<FMoveEventPayload>())
	{
		PresentMove(Event, *MovePayload);
	}
	else if (const FTriggerEventPayload* TriggerPayload = Event.Payload.GetPtr<FTriggerEventPayload>())
	{
		PresentTrigger(*TriggerPayload);
	}
	else if (const FCombatEndPayload* CombatEndPayload = Event.Payload.GetPtr<FCombatEndPayload>())
	{
		PresentCombatEnd(*CombatEndPayload);
	}
	else
	{
		// 알 수 없는 이벤트 타입 → 건너뛴다
		NotifyPresentationFinished();
	}
}

void UCombatEventComponent::ForceFinishPresentation()
{
	UE_LOG(LogTemp, Warning, TEXT("[CombatEvent] 연출 타임아웃 - 강제 진행 (남은 동시 연출 %d)"), PendingFinishCount);
	PendingFinishCount = 0; // 타임아웃은 카운터를 무시하고 넘긴다
	NotifyPresentationFinished();
}

void UCombatEventComponent::NotifyPresentationFinished()
{
	if (!bIsProcessing)
	{
		return;
	}

	// 동시 연출 중이면 마지막 하나가 끝날 때까지 대기
	if (PendingFinishCount > 1)
	{
		--PendingFinishCount;
		return;
	}
	PendingFinishCount = 0;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PresentationTimeoutHandle);
	}
	ProcessNextQueuedEvent();
}

void UCombatEventComponent::BeginAction()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	++ActionDepth;
}

void UCombatEventComponent::EndAction()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (ActionDepth <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Q] EndAction이 BeginActino 없이 호출됨."));
		return;
	}

	--ActionDepth;

	// 액션이 끝났는데, 보류중인 여파가 있으면 흘려보낸다.
	if (ActionDepth > 0)
	{
		return;
	}

	FlushPendingReactions();

	// 이 행동의 이벤트가 전부 나갔다. 결과를 보고 판단할 쪽(승리 판정 등)에 알린다
	OnActionFinished.Broadcast();
}


void UCombatEventComponent::PushReactionEvent(const FCombatEvent& NewEvent)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (ActionDepth > 0)
	{
		// 주 행동이 끝날 때까지 보류
		PendingReactions.Add(NewEvent);
		return;
	}

	PushEvent(NewEvent);
}

void UCombatEventComponent::PushDeathReaction(const FUnitDeathEntry& Entry)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 1) 보류 중이고 보류함 마지막이 사망 묶음이면 거기에 합친다.
	if (ActionDepth > 0 && PendingReactions.Num() > 0)
	{
		if (FUnitsDiedPayload* Batch = PendingReactions.Last().Payload.GetMutablePtr<FUnitsDiedPayload>())
		{
			Batch->Deaths.Add(Entry);
			return;
		}
	}

	// 2) 아니면 새 묶음을 만든다.
	FUnitsDiedPayload NewBatch;
	NewBatch.Deaths.Add(Entry);

	FCombatEvent BatchEvent;
	BatchEvent.Source = Entry.Unit; // 로그용. 연출은 Deaths 배열을 쓴다
	BatchEvent.Payload = FInstancedStruct::Make(NewBatch);

	// 3) 보류할지 바로 보낼지는 기존 경로가 판단한다.
	PushReactionEvent(BatchEvent);
}

void UCombatEventComponent::FlushPendingReactions()
{
	// 흘려보내는 도중, 새 여파가 생길 수 있으므로, 루프를 돌면서 계속 흘려보낸다.
	int32 Guard = 0;

	while (PendingReactions.Num() > 0)
	{
		if (++Guard > MaxReactionChain)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[Q] 여파 이벤트 체인이 너무 깊게 쌓임. %d개 이상은 허용되지 않음. 나머지 %d개는 무시됨."), 
				MaxReactionChain, PendingReactions.Num());
			PendingReactions.Reset();
			break;
		}

		// Batch 로 옮겨서, PushEvent 중에 새 여파가 생겨도 PendingReactions에 쌓이도록 한다.
		TArray<FCombatEvent> Batch = MoveTemp(PendingReactions);
		PendingReactions.Reset();

		for (const FCombatEvent& Event : Batch)
		{
			PushEvent(Event);
		}
	}
}

void UCombatEventComponent::SetProcessing(bool bNewProcessing)
{
	if (bIsProcessing != bNewProcessing)
	{
		bIsProcessing = bNewProcessing;
		OnPresentingChanged.Broadcast(bIsProcessing);
	}
}

void UCombatEventComponent::PresentSkill(const FCombatEvent& Event, const FSkillEventPayload& Payload)
{
	AUnit* Caster = Event.Source.Get();
	USkillComponent* SkillComp = Caster ? Caster->GetSkillComponent() : nullptr;

	USkillBase* Skill = nullptr;
	if (SkillComp)
	{
		if (Payload.SkillAsset)
		{
			Skill = SkillComp->GetOrCreateInstance(Payload.SkillAsset);
		}
		else
		{
			UE_LOG(LogSP, Warning, TEXT("[Q] 스킬 페이로드에 SkillAsset 없음 → 슬롯 태그로 찾음 (%s)"),
				*Payload.SkillTag.ToString());
			Skill = SkillComp->FindSkillByTag(Payload.SkillTag);
		}
	}

	if (Skill)
	{
		Skill->PresentSkillEffect(Event);
	}
	else
	{
		NotifyPresentationFinished();
	}
}

void UCombatEventComponent::PresentDeaths(const FUnitsDiedPayload& Payload)
{
	TArray<AUnit*> DyingUnits;
	for (const FUnitDeathEntry& Entry : Payload.Deaths)
	{
		if (AUnit* Unit = Entry.Unit.Get())
		{
			DyingUnits.Add(Unit);
		}
	}

	if (DyingUnits.Num() == 0)
	{
		NotifyPresentationFinished();
		return;
	}

	// 카운터를 먼저 세팅해야 몽타주 없는 유닛이 즉시 Notify해도 안전하다.
	PendingFinishCount = DyingUnits.Num();
	for (AUnit* Unit : DyingUnits)
	{
		Unit->PresentDeath();
	}
}

void UCombatEventComponent::PresentRevive(const FCombatEvent& Event)
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

void UCombatEventComponent::PresentMove(const FCombatEvent& Event, const FMoveEventPayload& Payload)
{
	if (AUnit* Target = Event.Source.Get())
	{
		Target->PresentMoveSegment(Payload.Waypoints);
	}
	else
	{
		NotifyPresentationFinished();
	}
}

void UCombatEventComponent::PresentTrigger(const FTriggerEventPayload& Payload)
{
	if (Payload.Trigger.GetObject())
	{
		ITileTrigger::Execute_PresentTriggerEffect(Payload.Trigger.GetObject(), Payload.Result);
	}
	else
	{
		NotifyPresentationFinished();
	}
}

void UCombatEventComponent::PresentCombatEnd(const FCombatEndPayload& Payload)
{
	// 지금은 기록만 하고 바로 끝. 나중에 "VICTORY" 배너를 넣으면
	// 배너가 끝날 때 기록 + NotifyPresentationFinished로 옮긴다
	if (URunProgressComponent* RunProgress = URunProgressComponent::Get(this))
	{
		RunProgress->MarkCombatEndPresented(Payload.StageSerial);
	}
	NotifyPresentationFinished();
}

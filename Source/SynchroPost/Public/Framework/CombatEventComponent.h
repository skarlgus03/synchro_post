#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/SPCombatEventStructure.h"
#include "CombatEventComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API UCombatEventComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatEventComponent();

	// 서버에서 호출 - 새 이벤트를 모두에게 방송
	UFUNCTION(BlueprintCallable, Category = "Combat Event")
	void PushEvent(const FCombatEvent& NewEvent);

	// 하나의 연출이 끝났을 때 호출 (연출 담당 쪽에서 콜백으로 불러줌)
	UFUNCTION(BlueprintCallable, Category = "Combat Event")
	void NotifyPresentationFinished();


	/*
	* 주 행동을 감싸고, 이 사이에 발생한 여파는 보류되었다가 
	* 주 행동이 만든 이벤트가 전부 큐에 들어간 뒤에 흘려보내진다.
	*/
	void BeginAction();
	void EndAction();

	/* 
	* 여파 이벤트(사망, 부활, 반격, 등).현재 액션중이면 보류, 아니면 즉시 Push한다.
	* 현재 액션이 아니라, 다른 추가적인 여파는 모두 이쪽으로 들어오게 된다.
	*/
	void PushReactionEvent(const FCombatEvent& NewEvent);


protected:
	virtual void BeginPlay() override;

	// 서버 → 모든 클라(서버 자신 포함) 방송용, 저장 안 됨
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPresentCombatEvent(const FCombatEvent& Event);
	
	void ProcessNextQueuedEvent();

	void FlushPendingReactions();


protected:

	// 로컬 재생 대기열 - 리플리케이트 아님, 각자 자기 컴퓨터에서만 관리
	UPROPERTY()
	TArray<FCombatEvent> LocalPresentationQueue;

	bool bIsProcessing = false;

	FTimerHandle PresentationTimeoutHandle;
	void ForceFinishPresentation();
	// 연출이 끝나지 않고 너무 오래 걸리면 강제로 끝내기 위한 타이머
	static constexpr float PresentationTimeoutSeconds = 8.f;

	int32 ActionDepth = 0;

	// 서버에서만 사용되는 펜딩 리액션 배열
	UPROPERTY()
	TArray<FCombatEvent> PendingReactions;

	// 무한으로 쌓이는 것을 방지하기 위해 걸어놓는 제한.
	static constexpr int32 MaxReactionChain = 16;
};

/*
* 중간에 return이 일어나도 EndAction이 호출되도록 보장하는 스코프 가드
* BeginAction/EndAction은 반드시 쌍으로 호출되어야 한다.
*/
struct SYNCHROPOST_API FCombatActionScope
{
	explicit FCombatActionScope(UCombatEventComponent* InComp)
		: Comp(InComp)
	{
		if (Comp) { Comp->BeginAction(); }
	}

	~FCombatActionScope()
	{
		if (Comp) { Comp->EndAction(); }
	}

	FCombatActionScope(const FCombatActionScope&) = delete;
	FCombatActionScope& operator=(const FCombatActionScope&) = delete;

private:
	UCombatEventComponent* Comp = nullptr;
};
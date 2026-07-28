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

protected:
	virtual void BeginPlay() override;

	// 서버 → 모든 클라(서버 자신 포함) 방송용, 저장 안 됨
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPresentCombatEvent(const FCombatEvent& Event);

	// 로컬 재생 대기열 - 리플리케이트 아님, 각자 자기 컴퓨터에서만 관리
	UPROPERTY()
	TArray<FCombatEvent> LocalPresentationQueue;

	bool bIsProcessing = false;

	void ProcessNextQueuedEvent();

};

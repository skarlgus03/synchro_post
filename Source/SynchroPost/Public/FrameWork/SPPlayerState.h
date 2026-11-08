
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SPPlayerState.generated.h"


UCLASS()
class SYNCHROPOST_API ASPPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	ASPPlayerState();

	void BeginPlay() override;


	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 이 플레이어가 준비한 스테이지 번호. 현재 StageSerial과 같아야 준비 완료로 간주. 0 = 준비 안함
	int32 GetReadyStageSerial() const { return ReadyStageSerial; }

	// 서버 전용
	void SetReadyStageSerial(int32 NewSerial);

protected:

	UPROPERTY(ReplicatedUsing = OnRep_ReadyStageSerial)
	int32 ReadyStageSerial = 0;

	UFUNCTION()
	void OnRep_ReadyStageSerial();
};
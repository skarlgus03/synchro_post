#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Types/SPGameFlowStructure.h"
#include "SPHUD.generated.h"

class UHUDLayoutWidget;
class UUnitSlot;
class AUnit;

/**
 * 로컬 플레이어 화면 담당. 틀 위젯을 만들고, "지금 상태"로 무엇을 보일지 정한다.
 * 입력·선택·액션 모드·RPC는 PlayerController 몫. 여기는 보여주기만 한다.
 */
UCLASS()
class SYNCHROPOST_API ASPHUD : public AHUD
{
	GENERATED_BODY()

public:
	// 상태에서 처음부터 다시 판단해 화면을 맞춘다. 어디서 몇 번 불러도 안전
	void RefreshLayout();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHUDLayoutWidget> LayoutWidgetClass;

	UPROPERTY()
	TObjectPtr<UHUDLayoutWidget> LayoutWidget;

private:
	// GameState가 복제되어 와야 틀을 띄울 수 있다 (자식 창이 Construct 때 구독하므로). 없으면 다음 틱에 재시도
	void TryInitialize();

	UFUNCTION() void HandleRunPhaseChanged(ERunPhase NewPhase);
	UFUNCTION() void HandlePlayerProgressChanged();
	UFUNCTION() void HandlePresentingChanged(bool bPresenting);
	UFUNCTION() void HandleSlotOwnerChanged(UUnitSlot* Slot);
	UFUNCTION() void HandleUnitTurnChanged(AUnit* Unit);	// 턴 시작·끝 둘 다
};
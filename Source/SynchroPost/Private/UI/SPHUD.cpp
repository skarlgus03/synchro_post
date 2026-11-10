#include "UI/SPHUD.h"
#include "UI/HUDLayoutWidget.h"
#include "UI/CombatActionWidget.h"
#include "UI/DeployWidget.h"
#include "UI/ResultWidget.h"
#include "UI/NodeSelectionWidget.h"
#include "Framework/SPGameState.h"
#include "Framework/SPPlayerController.h"
#include "Framework/SPPlayerState.h"
#include "Framework/RunProgressComponent.h"
#include "Framework/TurnStateComponent.h"
#include "Framework/CombatEventComponent.h"
#include "Slot/UnitSlotComponent.h"
#include "TimerManager.h"
#include "SynchroPost.h"

namespace
{
	// 표시 상태를 맞추고, "방금 열렸으면" true를 돌려준다 (열릴 때 한 번 새로 고치기 위해)
	bool SetWindowShown(UWidget* Window, bool bShow)
	{
		if (!Window)
		{
			return false;
		}
		const bool bWasShown = Window->GetVisibility() != ESlateVisibility::Collapsed;
		if (bWasShown == bShow)
		{
			return false;
		}
		Window->SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		return bShow;
	}
}

void ASPHUD::BeginPlay()
{
	Super::BeginPlay();
	TryInitialize();
}

void ASPHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (LayoutWidget)
	{
		LayoutWidget->RemoveFromParent();
		LayoutWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ASPHUD::TryInitialize()
{
	ASPGameState* SPGameState = GetWorld()->GetGameState<ASPGameState>();
	URunProgressComponent* RunProgress = SPGameState ? SPGameState->GetRunProgressComponent() : nullptr;
	if (!RunProgress)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ASPHUD::TryInitialize);
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !LayoutWidgetClass)
	{
		UE_LOG(LogSP, Warning, TEXT("[HUD] LayoutWidgetClass 미설정 또는 PC 없음"));
		return;
	}

	// 소유자를 PC로: 창들의 GetOwningPlayer()가 지금처럼 PC를 돌려준다
	LayoutWidget = CreateWidget<UHUDLayoutWidget>(PC, LayoutWidgetClass);
	if (!LayoutWidget)
	{
		return;
	}
	LayoutWidget->AddToViewport();	// 이 순간 자식 창들의 NativeConstruct가 돌며 RunProgress를 구독한다

	// 디자이너에서 뭘로 두었든 닫힌 상태에서 시작 → 첫 RefreshLayout이 "열림"을 정확히 감지
	SetWindowShown(LayoutWidget->DeployWindow, false);
	SetWindowShown(LayoutWidget->ResultWindow, false);
	SetWindowShown(LayoutWidget->NodeWindow, false);

	RunProgress->OnRunPhaseChanged.AddDynamic(this, &ASPHUD::HandleRunPhaseChanged);
	RunProgress->OnPlayerProgressChanged.AddDynamic(this, &ASPHUD::HandlePlayerProgressChanged);

	if (UTurnStateComponent* TurnState = SPGameState->GetTurnStateComponent())
	{
		TurnState->OnUnitTurnStart.AddDynamic(this, &ASPHUD::HandleUnitTurnChanged);
		TurnState->OnUnitTurnEnd.AddDynamic(this, &ASPHUD::HandleUnitTurnChanged);
	}
	if (UCombatEventComponent* CombatEvent = SPGameState->GetCombatEventComponent())
	{
		CombatEvent->OnPresentingChanged.AddDynamic(this, &ASPHUD::HandlePresentingChanged);
	}
	if (UUnitSlotComponent* UnitSlot = SPGameState->GetUnitSlotComponent())
	{
		UnitSlot->OnSlotOwnerChanged.AddDynamic(this, &ASPHUD::HandleSlotOwnerChanged);
	}

	// 늦게 들어왔을 수 있다: "바뀌었을 때"가 아니라 "지금 상태"로 시작
	RefreshLayout();
}

void ASPHUD::RefreshLayout()
{
	ASPPlayerController* PC = Cast<ASPPlayerController>(GetOwningPlayerController());
	const URunProgressComponent* RunProgress = URunProgressComponent::Get(this);
	if (!LayoutWidget || !PC || !RunProgress)
	{
		return;
	}

	// ── 행동 패널: 항상 보이고 상태만 바뀐다 ──
	if (LayoutWidget->CombatAction)
	{
		LayoutWidget->CombatAction->SetPanelState(PC->GetPanelUnit(), PC->GetPanelBlockReason());
	}

	// ── 단계별 창 ──
	const ERunPhase Phase = RunProgress->GetRunPhase();
	const bool bPostCombat = Phase == ERunPhase::NodeSelect && RunProgress->IsCombatEndPresentedLocally();
	const bool bMyRewardDone = bPostCombat && RunProgress->IsRewardDone(PC->GetPlayerState<ASPPlayerState>());

	if (SetWindowShown(LayoutWidget->DeployWindow, Phase == ERunPhase::Deploy))
	{
		LayoutWidget->DeployWindow->RefreshAll();
	}
	if (SetWindowShown(LayoutWidget->ResultWindow, bPostCombat && !bMyRewardDone))
	{
		LayoutWidget->ResultWindow->RefreshAll();
	}
	if (SetWindowShown(LayoutWidget->NodeWindow, bPostCombat && bMyRewardDone))
	{
		LayoutWidget->NodeWindow->RefreshNodeButtons();
		UE_LOG(LogSP, Log, TEXT("[NodeSelect] 창 표시 (버튼 %d개)"), RunProgress->GetReachableNodes().Num());
	}
}

void ASPHUD::HandleRunPhaseChanged(ERunPhase NewPhase) { RefreshLayout(); }
void ASPHUD::HandlePlayerProgressChanged() { RefreshLayout(); }
void ASPHUD::HandlePresentingChanged(bool bPresenting) { RefreshLayout(); }
void ASPHUD::HandleSlotOwnerChanged(UUnitSlot* Slot) { RefreshLayout(); }
void ASPHUD::HandleUnitTurnChanged(AUnit* Unit) { RefreshLayout(); }
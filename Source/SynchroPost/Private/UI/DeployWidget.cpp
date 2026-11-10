#include "UI/DeployWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "GameFramework/GameStateBase.h"
#include "UI/PlayerStatusRow.h"
#include "Framework/RunProgressComponent.h"
#include "Framework/SPPlayerController.h"
#include "Framework/SPPlayerState.h"



void UDeployWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ReadyButton)
	{
		ReadyButton->OnClicked.AddDynamic(this, &UDeployWidget::HandleReadyClicked);
	}
}

void UDeployWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// HUD틀이 살아 있는 동안 듣는다. (숨겨져도)
	if (URunProgressComponent* RunProgress = URunProgressComponent::Get(this))
	{
		RunProgress->OnPlayerProgressChanged.AddUniqueDynamic(this, &UDeployWidget::HandlePlayerProgressChanged);
		RunProgress->OnLocalDeployReadyChanged.AddUniqueDynamic(this, &UDeployWidget::HandleLocalDeployReadyChanged);
	}
	RefreshAll();	// 열리는 순간 항상 정확하게 시작
}

void UDeployWidget::NativeDestruct()
{
	if (URunProgressComponent* RunProgress = URunProgressComponent::Get(this))
	{
		RunProgress->OnPlayerProgressChanged.RemoveDynamic(this, &UDeployWidget::HandlePlayerProgressChanged);
		RunProgress->OnLocalDeployReadyChanged.RemoveDynamic(this, &UDeployWidget::HandleLocalDeployReadyChanged);
	}
	Super::NativeDestruct();
}

void UDeployWidget::HandlePlayerProgressChanged()
{
	RefreshAll();
}

void UDeployWidget::HandleLocalDeployReadyChanged(bool bCanReady)
{
	RefreshAll();
}

bool UDeployWidget::IsLocalPlayerReady() const
{
	const URunProgressComponent* RunProgress = URunProgressComponent::Get(this);
	const APlayerController* PC = GetOwningPlayer();
	const ASPPlayerState* PS = PC ? PC->GetPlayerState<ASPPlayerState>() : nullptr;
	return RunProgress && PS && PS->GetReadyStageSerial() == RunProgress->GetStageLoadSerial();
}

void UDeployWidget::RefreshAll()
{
	const URunProgressComponent* RunProgress = URunProgressComponent::Get(this);
	if (!RunProgress)
	{
		return;
	}
	const int32 StageSerial = RunProgress->GetStageLoadSerial();

	// ── 버튼 ──────────────────────────────────────────
	const bool bCanReady = RunProgress->CanLocalPlayerReady();
	const bool bReady = IsLocalPlayerReady();

	if (ReadyButton)
	{
		ReadyButton->SetIsEnabled(bCanReady);
	}
	if (ReadyButtonText)
	{
		ReadyButtonText->SetText(!bCanReady
			? NSLOCTEXT("Deploy", "Loading", "불러오는 중…")
			: (bReady ? NSLOCTEXT("Deploy", "Cancel", "준비 취소") : NSLOCTEXT("Deploy", "Ready", "준비")));
	}

	// ── 플레이어 줄 ───────────────────────────────────
	UPlayerStatusRow::SyncPlayerList(this, PlayerList, PlayerRowClass,
		[StageSerial](const ASPPlayerState& PS) { return PS.GetReadyStageSerial() == StageSerial; },
		NSLOCTEXT("Deploy", "RowReady", "준비 완료"),
		NSLOCTEXT("Deploy", "RowWaiting", "준비 중"));
}

void UDeployWidget::HandleReadyClicked()
{
	const URunProgressComponent* RunProgress = URunProgressComponent::Get(this);
	ASPPlayerController* PC = Cast<ASPPlayerController>(GetOwningPlayer());
	if (RunProgress && PC)
	{
		// 토글: 준비돼 있으면 취소, 아니면 준비
		PC->Server_SetReady(RunProgress->GetStageLoadSerial(), !IsLocalPlayerReady());
	}
}
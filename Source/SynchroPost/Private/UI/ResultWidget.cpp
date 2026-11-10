#include "UI/ResultWidget.h"

#include "Components/Button.h"
#include "UI/PlayerStatusRow.h"
#include "Framework/RunProgressComponent.h"
#include "Framework/SPPlayerController.h"
#include "Framework/SPPlayerState.h"

void UResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddDynamic(this, &UResultWidget::HandleConfirmClicked);
	}
}

void UResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (URunProgressComponent* RunProgress = URunProgressComponent::Get(this))
	{
		RunProgress->OnPlayerProgressChanged.AddUniqueDynamic(this, &UResultWidget::HandlePlayerProgressChanged);
	}
	RefreshAll();
}

void UResultWidget::NativeDestruct()
{
	if (URunProgressComponent* RunProgress = URunProgressComponent::Get(this))
	{
		RunProgress->OnPlayerProgressChanged.RemoveDynamic(this, &UResultWidget::HandlePlayerProgressChanged);
	}
	Super::NativeDestruct();
}

void UResultWidget::HandlePlayerProgressChanged()
{
	RefreshAll();
}

void UResultWidget::RefreshAll()
{
	const URunProgressComponent* RunProgress = URunProgressComponent::Get(this);
	if (!RunProgress)
	{
		return;
	}

	const APlayerController* PC = GetOwningPlayer();
	const ASPPlayerState* MyPS = PC ? PC->GetPlayerState<ASPPlayerState>() : nullptr;

	if (ConfirmButton)
	{
		// 이미 확인했으면 잠금 (창은 PC가 투표 창으로 바꿔준다)
		ConfirmButton->SetIsEnabled(!RunProgress->IsRewardDone(MyPS));
	}

	UPlayerStatusRow::SyncPlayerList(this, PlayerList, PlayerRowClass,
		[RunProgress](const ASPPlayerState& PS) { return RunProgress->IsRewardDone(&PS); },
		NSLOCTEXT("Result", "RowDone", "확인"),
		NSLOCTEXT("Result", "RowPending", "확인 중"));
}

void UResultWidget::HandleConfirmClicked()
{
	const URunProgressComponent* RunProgress = URunProgressComponent::Get(this);
	ASPPlayerController* PC = Cast<ASPPlayerController>(GetOwningPlayer());
	if (RunProgress && PC)
	{
		PC->Server_FinishReward(RunProgress->GetStageLoadSerial());
	}
	// 연타 방지. 서버가 완료를 복제하면 창이 바뀐다
	if (ConfirmButton)
	{
		ConfirmButton->SetIsEnabled(false);
	}
}
#include "UI/DeployWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "GameFramework/GameStateBase.h"
#include "UI/DeployPlayerRow.h"
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

	// 창이 열려 있는 동안만 듣는다
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
	if (!PlayerList || !PlayerRowClass)
	{
		return;
	}

	TArray<const ASPPlayerState*> Players;
	if (const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		for (const APlayerState* PS : GameState->PlayerArray)
		{
			if (const ASPPlayerState* SPPS = Cast<ASPPlayerState>(PS))
			{
				Players.Add(SPPS);
			}
		}
	}

	// 줄 수를 플레이어 수에 맞춘다 (있는 줄은 재사용)
	while (PlayerList->GetChildrenCount() < Players.Num())
	{
		PlayerList->AddChild(CreateWidget<UDeployPlayerRow>(this, PlayerRowClass));
	}
	while (PlayerList->GetChildrenCount() > Players.Num())
	{
		PlayerList->RemoveChildAt(PlayerList->GetChildrenCount() - 1);
	}

	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		if (UDeployPlayerRow* Row = Cast<UDeployPlayerRow>(PlayerList->GetChildAt(Index)))
		{
			Row->SetPlayer(Players[Index]->GetPlayerName(), Players[Index]->GetReadyStageSerial() == StageSerial);
		}
	}
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
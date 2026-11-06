
#include "UI/NodeSelectionWidget.h"
#include "UI/NodeButtonWidget.h"
#include "Components/HorizontalBox.h"
#include "Framework/RunProgressComponent.h"
#include "Framework/SPPlayerController.h"
#include "Components/TextBlock.h"

void UNodeSelectionWidget::RefreshNodeButtons()
{
	if (!NodeButtonContainer || !NodeButtonWidgetClass)
	{
		return;
	}
	NodeButtonContainer->ClearChildren();
	SetIsEnabled(true);

	const URunProgressComponent* RunProgress = URunProgressComponent::Get(this);
	if (!RunProgress)
	{
		return;
	}

	// 서버 그래프가 아니라 복제된 선택지를 읽는다 (원격 클라에는 그래프가 없다)
	const bool bCanChoose = URunProgressComponent::CanChooseNode(GetOwningPlayer());

	for (const FNodeChoice& Choice : RunProgress->GetReachableNodes())
	{
		UNodeButtonWidget* NodeButton = CreateWidget<UNodeButtonWidget>(this, NodeButtonWidgetClass);
		if (!NodeButton)
		{
			continue;
		}
		// BP에서 버튼 글자/아이콘을 갱신하도록 Choice를 전달하고 선택 가능 여부를 설정한다
		NodeButton->SetNodeChoice(Choice);
		NodeButton->SetIsEnabled(bCanChoose);
		NodeButton->OnNodeSelected.AddDynamic(this, &UNodeSelectionWidget::HandleNodeSelected);
		NodeButtonContainer->AddChild(NodeButton);
	}

	if (WaitingText)
	{
		WaitingText->SetVisibility(bCanChoose ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UNodeSelectionWidget::HandleNodeSelected(int32 NodeIndex)
{
	const URunProgressComponent* RunProgress = URunProgressComponent::Get(this);
	
	ASPPlayerController* PC = Cast<ASPPlayerController>(GetOwningPlayer());
	if (PC && RunProgress)
	{
		PC->Server_RequestEnterNode(RunProgress->GetStageLoadSerial(), NodeIndex);
	}

	// 창을 직접 닫지 않는다. 서버가 단계를 Deploy로 바꾸면 PC가 닫는다.
	// 그 사이 연타를 막기 위해 잠시 비활성화만
	SetIsEnabled(false);
}
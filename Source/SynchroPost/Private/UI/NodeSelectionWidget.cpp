
#include "UI/NodeSelectionWidget.h"
#include "UI/NodeButtonWidget.h"
#include "Components/VerticalBox.h"
#include "Framework/RunProgressSubsystem.h"
#include "Framework/SPPlayerController.h"

void UNodeSelectionWidget::RefreshNodeButtons()
{
	if (!NodeButtonContainer || !NodeButtonWidgetClass)
	{
		return;
	}
	NodeButtonContainer->ClearChildren();

	URunProgressSubsystem* RunProgressSubsystem = GetGameInstance()->GetSubsystem<URunProgressSubsystem>();

	if (!RunProgressSubsystem)
	{
		return;
	}

	for (int32 NodeIndex : RunProgressSubsystem->GetReachableNodeIndices())
	{
		UNodeButtonWidget* NodeButton = CreateWidget<UNodeButtonWidget>(this, NodeButtonWidgetClass);
		if (NodeButton)
		{
			NodeButton->SetNodeIndex(NodeIndex);
			NodeButton->OnNodeSelected.AddDynamic(this, &UNodeSelectionWidget::HandleNodeSelected);
			NodeButtonContainer->AddChild(NodeButton);
		}
	}
}

void UNodeSelectionWidget::HandleNodeSelected(int32 NodeIndex)
{
	if (ASPPlayerController* PC = Cast<ASPPlayerController>(GetOwningPlayer()))
	{
		PC->Server_RequestEnterNode(NodeIndex);
	}

	RemoveFromParent();
}

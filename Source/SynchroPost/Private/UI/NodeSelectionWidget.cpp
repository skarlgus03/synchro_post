
#include "UI/NodeSelectionWidget.h"
#include "UI/NodeButtonWidget.h"
#include "Components/HorizontalBox.h"
#include "Framework/RunProgressComponent.h"
#include "Framework/SPPlayerController.h"

void UNodeSelectionWidget::RefreshNodeButtons()
{
	if (!NodeButtonContainer || !NodeButtonWidgetClass)
	{
		return;
	}
	NodeButtonContainer->ClearChildren();

	URunProgressComponent* RunProgressComponent = URunProgressComponent::Get(this);

	if (!RunProgressComponent)
	{
		return;
	}

	for (int32 NodeIndex : RunProgressComponent->GetReachableNodeIndices())
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

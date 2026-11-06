#include "UI/NodeButtonWidget.h"
#include "Components/Button.h"
void UNodeButtonWidget::SetNodeChoice(const FNodeChoice& InChoice)
{
	NodeChoice = InChoice;
	OnNodeChoiceSet(NodeChoice);
}

void UNodeButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &UNodeButtonWidget::HandleClicked);
	}
}

void UNodeButtonWidget::HandleClicked()
{
	OnNodeSelected.Broadcast(NodeChoice.NodeIndex);
}
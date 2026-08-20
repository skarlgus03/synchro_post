#include "UI/NodeButtonWidget.h"
#include "Components/Button.h"

void UNodeButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &UNodeButtonWidget::HandleClicked);
	}
}

void UNodeButtonWidget::HandleClicked()
{
	OnNodeSelected.Broadcast(NodeIndex);
}
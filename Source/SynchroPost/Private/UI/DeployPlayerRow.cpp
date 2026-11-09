#include "UI/DeployPlayerRow.h"
#include "Components/TextBlock.h"

void UDeployPlayerRow::SetPlayer(const FString& PlayerName, bool bReady)
{
	if (NameText)
	{
		NameText->SetText(FText::FromString(PlayerName));
	}
	if (StatusText)
	{
		StatusText->SetText(bReady
			? NSLOCTEXT("Deploy", "RowReady", "준비 완료")
			: NSLOCTEXT("Deploy", "RowWaiting", "준비 중"));
	}
	OnReadyStateSet(bReady);
}
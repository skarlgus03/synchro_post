#include "UI/PlayerStatusRow.h"

#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "GameFramework/GameStateBase.h"
#include "Framework/SPPlayerState.h"

void UPlayerStatusRow::SetPlayer(const FString& PlayerName, bool bDone, const FText& InStatusText)
{
	if (NameText)
	{
		NameText->SetText(FText::FromString(PlayerName));
	}
	if (StatusText)
	{
		StatusText->SetText(InStatusText);
	}
	OnDoneStateSet(bDone);
}

void UPlayerStatusRow::SyncPlayerList(UUserWidget* Owner, UPanelWidget* List, TSubclassOf<UPlayerStatusRow> RowClass,
	TFunctionRef<bool(const ASPPlayerState&)> IsDone, const FText& DoneText, const FText& PendingText)
{
	if (!Owner || !List || !RowClass)
	{
		return;
	}

	TArray<const ASPPlayerState*> Players;
	if (const AGameStateBase* GameState = Owner->GetWorld() ? Owner->GetWorld()->GetGameState() : nullptr)
	{
		for (const APlayerState* PS : GameState->PlayerArray)
		{
			if (const ASPPlayerState* SPPS = Cast<ASPPlayerState>(PS))
			{
				Players.Add(SPPS);
			}
		}
	}

	while (List->GetChildrenCount() < Players.Num())
	{
		List->AddChild(CreateWidget<UPlayerStatusRow>(Owner, RowClass));
	}
	while (List->GetChildrenCount() > Players.Num())
	{
		List->RemoveChildAt(List->GetChildrenCount() - 1);
	}

	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		if (UPlayerStatusRow* Row = Cast<UPlayerStatusRow>(List->GetChildAt(Index)))
		{
			const bool bDone = IsDone(*Players[Index]);
			Row->SetPlayer(Players[Index]->GetPlayerName(), bDone, bDone ? DoneText : PendingText);
		}
	}
}
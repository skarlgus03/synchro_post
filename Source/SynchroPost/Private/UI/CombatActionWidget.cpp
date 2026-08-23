#include "UI/CombatActionWidget.h"
#include "Components/Button.h"
#include "Components/NamedSlot.h"
#include "Framework/SPPlayerController.h"

void UCombatActionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if(UnitSkillButton)
	{
		UnitSkillButton->OnClicked.AddDynamic(this, &UCombatActionWidget::HandleUnitSkillClicked);
	}
	if (PartySkillButton)
	{
		PartySkillButton->OnClicked.AddDynamic(this, &UCombatActionWidget::HandlePartySkillClicked);
	}
	if (ItemButton)
	{
		ItemButton->OnClicked.AddDynamic(this, &UCombatActionWidget::HandleItemClicked);
	}
	if (UtilityButton)
	{
		UtilityButton->OnClicked.AddDynamic(this, &UCombatActionWidget::HandleUtilityClicked);
	}
	if (EndTurnButton)
	{
		EndTurnButton->OnClicked.AddDynamic(this, &UCombatActionWidget::HandleEndTurnClicked);
	}

	ClearFlyout();
}

void UCombatActionWidget::HandleUnitSkillClicked()
{
	ShowFlyout(SkillFlyoutClass);
}

void UCombatActionWidget::HandlePartySkillClicked()
{
}

void UCombatActionWidget::HandleItemClicked()
{
}

void UCombatActionWidget::HandleUtilityClicked()
{
}

void UCombatActionWidget::HandleEndTurnClicked()
{
	ClearFlyout();
	if(ASPPlayerController* PC = Cast<ASPPlayerController>(GetOwningPlayer()))
	{
		PC->Server_RequestEndTurn();
	}
}

void UCombatActionWidget::ShowFlyout(TSubclassOf<UUserWidget> FlyoutClass)
{
	if (!FlyoutSlot || !FlyoutClass)
	{
		return;
	}

	UUserWidget* FlyoutWidget = FlyoutCache.FindRef(FlyoutClass);
	if (!FlyoutWidget)
	{
		FlyoutWidget = CreateWidget<UUserWidget>(GetWorld(), FlyoutClass);
		if (FlyoutWidget)
		{
			FlyoutCache.Add(FlyoutClass, FlyoutWidget);
		}
	}
	FlyoutSlot->ClearChildren();
	FlyoutSlot->AddChild(FlyoutWidget);
}

void UCombatActionWidget::ClearFlyout()
{
	if (FlyoutSlot)
	{
		FlyoutSlot->ClearChildren();
	}
}

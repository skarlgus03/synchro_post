#include "UI/CombatActionWidget.h"
#include "Components/Button.h"
#include "Components/NamedSlot.h"
#include "Framework/SPPlayerController.h"
#include "Unit/Unit.h"
#include "UI/SkillFlyoutWidget.h"

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
	if (USkillFlyoutWidget* SkillFlyout = Cast<USkillFlyoutWidget>(ShowFlyout(SkillFlyoutClass)))
	{
		SkillFlyout->SetUnit(PanelUnit.Get());
	}
}

void UCombatActionWidget::HandlePartySkillClicked()
{
}

void UCombatActionWidget::HandleItemClicked()
{
}

void UCombatActionWidget::HandleUtilityClicked()
{
	ClearFlyout();
	if (ASPPlayerController* PC = Cast<ASPPlayerController>(GetOwningPlayer()))
	{
		PC->EnterMoveMode();
	}
}

void UCombatActionWidget::HandleEndTurnClicked()
{
	ClearFlyout();
	if(ASPPlayerController* PC = Cast<ASPPlayerController>(GetOwningPlayer()))
	{
		PC->Server_RequestEndTurn();
	}
}

UUserWidget* UCombatActionWidget::ShowFlyout(TSubclassOf<UUserWidget> FlyoutClass)
{
	if (!FlyoutSlot || !FlyoutClass)
	{
		return nullptr;
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
	return FlyoutWidget;
}

void UCombatActionWidget::ClearFlyout()
{
	if (FlyoutSlot)
	{
		FlyoutSlot->ClearChildren();
	}
}

void UCombatActionWidget::SetPanelState(AUnit* InUnit, ECommandBlockReason InReason)
{
	if (PanelUnit.Get() != InUnit)
	{
		ClearFlyout();
	}
	PanelUnit = InUnit;
	BlockReason = InReason;
	

	const bool bUsable = (InReason == ECommandBlockReason::None);
	SetVisibility(bUsable ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	if (!bUsable)
	{
		ClearFlyout();
	}
}

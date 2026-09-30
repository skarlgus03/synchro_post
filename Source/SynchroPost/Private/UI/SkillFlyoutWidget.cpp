#include "UI/SkillFlyoutWidget.h"
#include "Components/PanelWidget.h"
#include "Unit/Unit.h"
#include "Unit/SkillComponent.h"
#include "Framework/SPPlayerController.h"
#include "UI/SkillButtonWidget.h"

void USkillFlyoutWidget::SetUnit(AUnit* Unit)
{
	RefreshSkillButtons(Unit);
}

void USkillFlyoutWidget::RefreshSkillButtons(AUnit* Unit)
{
	if (!SkillButtonContainer || !SkillButtonClass)
	{
		return;
	}
	SkillButtonContainer->ClearChildren();

	if (!Unit || !Unit->GetSkillComponent())
	{
		return;
	}

	USkillComponent* SkillComponent = Unit->GetSkillComponent();
	for (const FSkillEntry& Entry : SkillComponent->GetSkillEntries())
	{
		if (!Entry.Skill) continue;
		USkillButtonWidget* SkillButton = CreateWidget<USkillButtonWidget>(this, SkillButtonClass);
		if (SkillButton)
		{
			const FSkillData SkillData = SkillComponent->GetSkillData(Entry.SkillSlotTag);
			SkillButton->SetSkillInfo(Entry.SkillSlotTag, SkillData.SkillIcon);
			SkillButton->OnSkillSelected.AddDynamic(this, &USkillFlyoutWidget::HandleSkillSelected);

			SkillButtonContainer->AddChild(SkillButton);
		}
	}
}

void USkillFlyoutWidget::HandleSkillSelected(FGameplayTag SkillSlotTag)
{
	if (ASPPlayerController* PC = GetOwningPlayer<ASPPlayerController>())
	{
		PC->EnterSkillMode(SkillSlotTag);
	}
}

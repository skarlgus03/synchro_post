#include "UI/SkillFlyoutWidget.h"
#include "Components/PanelWidget.h"
#include "Unit/Unit.h"
#include "Framework/TurnManager.h"
#include "Unit/SkillComponent.h"
#include "Framework/SPPlayerController.h"
#include "UI/SkillButtonWidget.h"

void USkillFlyoutWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RefreshSkillButtons();
}

void USkillFlyoutWidget::RefreshSkillButtons()
{
	if (!SkillButtonContainer || !SkillButtonClass) return;
	SkillButtonContainer->ClearChildren();

	UTurnManager* TurnManager = GetWorld()->GetSubsystem<UTurnManager>();

	AUnit* CurrentUnit = TurnManager ? TurnManager->GetCurrentUnit() : nullptr;
	if (!CurrentUnit || !CurrentUnit->GetSkillComponent()) return;

	USkillComponent* SkillComponent = CurrentUnit->GetSkillComponent();

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

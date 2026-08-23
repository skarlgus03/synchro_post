
#include "UI/SkillButtonWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"

void USkillButtonWidget::SetSkillInfo(FGameplayTag InSkillSlotTag, UTexture2D* InSkillIcon)
{
	SkillSlotTag = InSkillSlotTag;
	if (SkillIcon && InSkillIcon)
	{
		SkillIcon->SetBrushFromTexture(InSkillIcon);
	}
}

void USkillButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &USkillButtonWidget::HandleClicked);
	}
}

void USkillButtonWidget::HandleClicked()
{
	OnSkillSelected.Broadcast(SkillSlotTag);
}
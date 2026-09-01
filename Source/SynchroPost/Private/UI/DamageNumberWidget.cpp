#include "UI/DamageNumberWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Types/SPGameplayTags.h"

void UDamageNumberWidget::SetupDamageNumber(int32 Amount, bool bIsCritical, const FGameplayTagContainer TypeTags)
{
	if (!AmountText)
	{
		return;
	}

	const bool bIsHeal = TypeTags.HasTag(SPTags::Heal);
	const FString Prefix = bIsHeal ? TEXT("+") : TEXT("-");
	AmountText->SetText(FText::FromString(Prefix + FString::FromInt(Amount)));

	FLinearColor Color = DefaultColor;
	for (const FGameplayTag& Tag : TypeTags)
	{
		if (const FLinearColor* FoundColor = ColorByDamageTag.Find(Tag))
		{
			Color = *FoundColor;
			break;   // 첫 매칭 태그 색 사용 (한 히트에 여러 속성 태그 섞이는 경우는 우선순위 필요할 수도 있음)
		}
	}	
	if (bIsCritical)
	{
		// 알파는 그대로 두고 RGB만 밝게
		Color.R *= CriticalBrightnessMultiplier;
		Color.G *= CriticalBrightnessMultiplier;
		Color.B *= CriticalBrightnessMultiplier;
	}
	AmountText->SetColorAndOpacity(FSlateColor(Color));

	if (CriticalIcon)
	{
		CriticalIcon->SetVisibility(bIsCritical ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UDamageNumberWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (FadeUpAnimation)
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UDamageNumberWidget::HandleFadeAnimationFinished);
		BindToAnimationFinished(FadeUpAnimation, FinishedEvent);

		PlayAnimation(FadeUpAnimation);
	}
	else
	{
		// 애니메이션이 아직 없으면 일정 시간 뒤 그냥 제거
		FTimerHandle UnusedHandle;
		GetWorld()->GetTimerManager().SetTimer(UnusedHandle, this, &UDamageNumberWidget::HandleFadeAnimationFinished, FallbackLifetime, false);
	}
}

void UDamageNumberWidget::HandleFadeAnimationFinished()
{
	RemoveFromParent();
}

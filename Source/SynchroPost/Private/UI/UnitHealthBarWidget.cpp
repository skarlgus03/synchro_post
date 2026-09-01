
#include "UI/UnitHealthBarWidget.h"
#include "UI/DamageNumberWidget.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void UUnitHealthBarWidget::InitializeHealthBar(int32 InitialHealth, int32 InMaxHealth)
{
	MaxHealth = FMath::Max(InMaxHealth, 1);
	DisplayedHealth = static_cast<float>(InitialHealth);
	TargetHealth = DisplayedHealth;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(DisplayedHealth / MaxHealth);
	}
}

void UUnitHealthBarWidget::AnimateToHealth(int32 NewHealth)
{
	TargetHealth = static_cast<float>(NewHealth);
}

void UUnitHealthBarWidget::ShowDamageNumber(int32 Amount, bool bIsCritical, const FGameplayTagContainer& TypeTags)
{
	if (!DamageNumberWidgetClass || !DamageNumberContainer)
	{
		return;
	}

	UDamageNumberWidget* NewNumber = CreateWidget<UDamageNumberWidget>(this, DamageNumberWidgetClass);
	if (!NewNumber)
	{
		return;
	}

	NewNumber->SetupDamageNumber(Amount, bIsCritical, TypeTags);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DamageNumberContainer->AddChild(NewNumber)))
	{
		const float RandomOffsetX = FMath::RandRange(-RandomOffsetRangeX, RandomOffsetRangeX);
		CanvasSlot->SetPosition(FVector2D(RandomOffsetX, 0.f));
		CanvasSlot->SetAutoSize(true);
	}
}

void UUnitHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!FMath::IsNearlyEqual(DisplayedHealth, TargetHealth) && HealthProgressBar)
	{
		DisplayedHealth = FMath::FInterpTo(DisplayedHealth, TargetHealth, InDeltaTime, InterpSpeed);
		HealthProgressBar->SetPercent(DisplayedHealth / MaxHealth);
	}
}

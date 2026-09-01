#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "UnitHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UPanelWidget;
class UDamageNumberWidget;

UCLASS()
class SYNCHROPOST_API UUnitHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()
	 
public:
	void InitializeHealthBar(int32 InitialHealth, int32 InMaxHealth);
	void AnimateToHealth(int32 NewHealth);
	void ShowDamageNumber(int32 Amount, bool bIsCritical, const FGameplayTagContainer& TypeTags);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> DamageNumberContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	TSubclassOf<UDamageNumberWidget> DamageNumberWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Health Bar")
	float InterpSpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	float RandomOffsetRangeX = 15.0f;

	int32 MaxHealth = 1;
	float DisplayedHealth = 0.f;
	float TargetHealth = 0.f;
};

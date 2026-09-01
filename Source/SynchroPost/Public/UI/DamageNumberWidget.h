
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;
class UImage;

UCLASS()
class SYNCHROPOST_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetupDamageNumber(int32 Amount, bool bIsCritical, const FGameplayTagContainer TypeTags);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleFadeAnimationFinished();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AmountText;

	UPROPERTY(Transient, meta = (BindWidgetAnim, OptionalWidget = true))
	TObjectPtr<UWidgetAnimation> FadeUpAnimation;

	// FadeUpAnimation을 아직 안 만들어놨을 때를 대비한 최소한의 표시 시간
	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	float FallbackLifetime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	TMap<FGameplayTag, FLinearColor> ColorByDamageTag;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	FLinearColor DefaultColor = FLinearColor::White;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CriticalIcon;  

	UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
	float CriticalBrightnessMultiplier = 1.3f;
};


#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "SkillButtonWidget.generated.h"

class UButton;
class UImage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillButtonClicked, FGameplayTag, SkillSlotTag);

UCLASS()
class SYNCHROPOST_API USkillButtonWidget : public UUserWidget
{
	GENERATED_BODY()
	

public:

	UPROPERTY(BlueprintAssignable)
	FOnSkillButtonClicked OnSkillSelected;

	void SetSkillInfo(FGameplayTag InSkillSlotTag, UTexture2D* InSkillIcon);

protected:

	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillIcon;

	FGameplayTag SkillSlotTag;
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "SkillFlyoutWidget.generated.h"

class UPanelWidget;
class USkillButtonWidget;

UCLASS()
class SYNCHROPOST_API USkillFlyoutWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected: 

	virtual void NativeOnInitialized() override;

	void RefreshSkillButtons();

	UFUNCTION() 
	void HandleSkillSelected(FGameplayTag SkillSlotTag);

	UPROPERTY(meta = (BindWidget)) 
	TObjectPtr<UPanelWidget> SkillButtonContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Combat UI") 
	TSubclassOf<USkillButtonWidget> SkillButtonClass;
};

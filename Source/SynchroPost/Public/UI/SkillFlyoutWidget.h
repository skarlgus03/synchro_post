#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "SkillFlyoutWidget.generated.h"

class UPanelWidget;
class USkillButtonWidget;
class AUnit;

UCLASS()
class SYNCHROPOST_API USkillFlyoutWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/*이 유닛의 스킬로 버튼을 다시 그림.*/
	void SetUnit(AUnit* Unit);
protected: 

	void RefreshSkillButtons(AUnit* Unit);

	UFUNCTION() 
	void HandleSkillSelected(FGameplayTag SkillSlotTag);

	UPROPERTY(meta = (BindWidget)) 
	TObjectPtr<UPanelWidget> SkillButtonContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Combat UI") 
	TSubclassOf<USkillButtonWidget> SkillButtonClass;
};

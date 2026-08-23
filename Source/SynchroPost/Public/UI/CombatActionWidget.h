
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CombatActionWidget.generated.h"

class UButton;
class UNamedSlot;

UCLASS()
class SYNCHROPOST_API UCombatActionWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	void ShowFlyout(TSubclassOf<UUserWidget> FlyoutClass);

	void ClearFlyout();

protected:
	virtual void NativeOnInitialized() override;


	UFUNCTION()	void HandleUnitSkillClicked();
	UFUNCTION()	void HandlePartySkillClicked();
	UFUNCTION()	void HandleItemClicked();
	UFUNCTION()	void HandleUtilityClicked();
	UFUNCTION()	void HandleEndTurnClicked();



	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> UnitSkillButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> PartySkillButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ItemButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> UtilityButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> EndTurnButton;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UNamedSlot> FlyoutSlot;


	// 카테고리별 플라이아웃 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Combat UI") TSubclassOf<UUserWidget> SkillFlyoutClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat UI") TSubclassOf<UUserWidget> PartySkillFlyoutClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat UI") TSubclassOf<UUserWidget> ItemFlyoutClass;
	UPROPERTY(EditDefaultsOnly, Category = "Combat UI") TSubclassOf<UUserWidget> UtilityFlyoutClass;


	// 매번 새로 만들지 않도록 캐싱 (선택 사항, 필요하면)
	UPROPERTY() TMap<TSubclassOf<UUserWidget>, TObjectPtr<UUserWidget>> FlyoutCache;

};

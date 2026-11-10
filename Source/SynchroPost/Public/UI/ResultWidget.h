#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ResultWidget.generated.h"

class UButton;
class UPanelWidget;
class UPlayerStatusRow;

/**
 * 전투 결과 창. 지금은 확인 버튼 + 플레이어별 확인 상태.
 * 나중에 보상 선택이 확인 버튼 자리에 들어간다.
 */
UCLASS()
class SYNCHROPOST_API UResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 상태에서 처음부터 다시 계산해서 버튼과 줄을 맞춘다. HUD가 창을 열 때도 부른다
	void RefreshAll();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UPanelWidget> PlayerList;

	UPROPERTY(EditDefaultsOnly, Category = "Result")
	TSubclassOf<UPlayerStatusRow> PlayerRowClass;

private:

	UFUNCTION()
	void HandlePlayerProgressChanged();

	UFUNCTION()
	void HandleConfirmClicked();
};
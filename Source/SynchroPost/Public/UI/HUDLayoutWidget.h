#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDLayoutWidget.generated.h"

class UCombatActionWidget;
class UDeployWidget;
class UResultWidget;
class UNodeSelectionWidget;

/**
 * 화면 전체 틀 (WBP_HUD의 부모).
 * 어디에 놓을지는 디자이너에서 정하고, 무엇을 보일지는 ASPHUD가 정한다.
 * 이 클래스는 로직이 없다. 자식 창을 이름으로 묶어 주기만 한다.
 */
UCLASS(Abstract)
class SYNCHROPOST_API UHUDLayoutWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCombatActionWidget> CombatAction;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UDeployWidget> DeployWindow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UResultWidget> ResultWindow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UNodeSelectionWidget> NodeWindow;
};
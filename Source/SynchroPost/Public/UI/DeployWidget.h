#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeployWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;
class UPlayerStatusRow;

/**
 * 배치 단계 창: 플레이어별 준비 상태 + 준비/취소 버튼.
 * 열려 있는 동안 RunProgress의 이벤트를 듣고, 이벤트가 오면 상태에서 전부 다시 계산한다.
 * (나중에 자유 배치 안내도 여기에 붙인다. 배치 가능 칸 표시는 GridVisualizer 몫)
 */
UCLASS()
class SYNCHROPOST_API UDeployWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 상태에서 처음부터 다시 계산해서 버튼과 줄을 맞춘다. HUD가 창을 열 때도 부른다
	void RefreshAll();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReadyButton;

	// "준비" / "준비 취소" / "불러오는 중…"
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ReadyButtonText;

	// 플레이어 줄을 담는 상자 (VerticalBox 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PlayerList;

	UPROPERTY(EditDefaultsOnly, Category = "Deploy")
	TSubclassOf<UPlayerStatusRow> PlayerRowClass;

private:
	

	UFUNCTION()
	void HandlePlayerProgressChanged();

	UFUNCTION()
	void HandleLocalDeployReadyChanged(bool bCanReady);

	UFUNCTION()
	void HandleReadyClicked();

	// 현재 로컬 플레이어가 준비 상태인지 확인한다
	bool IsLocalPlayerReady() const;
};
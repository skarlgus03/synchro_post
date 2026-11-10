#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatusRow.generated.h"

class UTextBlock;
class UPanelWidget;
class ASPPlayerState;

/**
 * 플레이어 한 줄: 이름 + 상태. 데이터를 받아 보여주기만 한다 (구독 없음).
 * 배치(준비), 결과(확인), 투표 창에서 같이 쓴다.
 */
UCLASS()
class SYNCHROPOST_API UPlayerStatusRow : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPlayer(const FString& PlayerName, bool bDone, const FText& StatusText);

	/**
	 * 목록 상자의 줄을 접속 중인 플레이어에 맞춘다 (있는 줄은 재사용).
	 * IsDone: 그 플레이어가 이번 단계 일을 끝냈나 / DoneText, PendingText: 상태 글자
	 */
	static void SyncPlayerList(UUserWidget* Owner, UPanelWidget* List, TSubclassOf<UPlayerStatusRow> RowClass,
		TFunctionRef<bool(const ASPPlayerState&)> IsDone, const FText& DoneText, const FText& PendingText);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	// BP에서 색·아이콘을 바꾼다 (기존 OnReadyStateSet에서 이름 변경)
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Status")
	void OnDoneStateSet(bool bDone);
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeployPlayerRow.generated.h"

class UTextBlock;

/** 배치 창의 플레이어 한 줄. 데이터를 받아 보여주기만 한다 (구독 없음) */
UCLASS()
class SYNCHROPOST_API UDeployPlayerRow : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPlayer(const FString& PlayerName, bool bReady);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	// BP에서 색·아이콘을 바꾼다
	UFUNCTION(BlueprintImplementableEvent, Category = "Deploy")
	void OnReadyStateSet(bool bReady);
};
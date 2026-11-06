
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NodeSelectionWidget.generated.h"

class UHorizontalBox;
class UNodeButtonWidget;


UCLASS()
class SYNCHROPOST_API UNodeSelectionWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Node Selection")
	void RefreshNodeButtons();

protected:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NodeButtonContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Node Selection")
	TSubclassOf<UNodeButtonWidget> NodeButtonWidgetClass;

	// (선택) "방장 선택중" 안내표시. BP에 없으면 무시됨
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> WaitingText;

	UFUNCTION()
	void HandleNodeSelected(int32 NodeIndex);
};

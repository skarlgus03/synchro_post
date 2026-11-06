#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SPGameFlowStructure.h"
#include "NodeButtonWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeButtonClicked, int32, NodeIndex);

UCLASS()
class SYNCHROPOST_API UNodeButtonWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(BlueprintAssignable)
	FOnNodeButtonClicked OnNodeSelected;

	void SetNodeChoice(const FNodeChoice& InChoice);

protected:

	virtual void NativeOnInitialized() override;

	// BP에서 버튼 글자/아이콘을 갱신함
	UFUNCTION(BlueprintImplementableEvent, Category = "Node Selection")
	void OnNodeChoiceSet(const FNodeChoice& Choice);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;

	UFUNCTION()
	void HandleClicked();

private:

	FNodeChoice NodeChoice;
};

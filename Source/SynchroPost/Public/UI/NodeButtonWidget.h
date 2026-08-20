#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

	void SetNodeIndex(int32 InNodeIndex) { NodeIndex = InNodeIndex; }

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;

	UFUNCTION()
	void HandleClicked();

private:

	int32 NodeIndex = -1;
};

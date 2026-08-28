
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SPPlayerState.generated.h"

class UItemDataAsset;

UCLASS()
class SYNCHROPOST_API ASPPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	ASPPlayerState();

	void BeginPlay() override;


protected:



	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Test")
	TObjectPtr<UItemDataAsset> TestItemDataAsset;
};

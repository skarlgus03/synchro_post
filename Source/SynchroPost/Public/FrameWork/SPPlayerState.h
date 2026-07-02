
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SPPlayerState.generated.h"

class UUnitSlotComponent;
class UItemDataAsset;

UCLASS()
class SYNCHROPOST_API ASPPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	ASPPlayerState();

	void BeginPlay() override;


	FORCEINLINE UUnitSlotComponent* GetUnitSlotComponent() const { return UnitSlotComponent; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUnitSlotComponent> UnitSlotComponent;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Test")
	TObjectPtr<UItemDataAsset> TestItemDataAsset;
};

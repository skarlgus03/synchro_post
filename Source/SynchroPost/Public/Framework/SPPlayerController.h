#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SPPlayerController.generated.h"

class AGridVisualizer;
class UTileMapDataAsset;

UCLASS()
class SYNCHROPOST_API ASPPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	
public:
	ASPPlayerController();

	virtual void BeginPlay() override;

	FORCEINLINE AGridVisualizer* GetGridVisualizer() const { return GridVisualizer; }
	
protected:

	virtual void Tick(float DeltaSeconds) override;

	FIntPoint LastHoveredCoord = FIntPoint(MIN_int32, MIN_int32);

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Grid Visualizer")
	TSubclassOf<AGridVisualizer> GridVisualizerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Grid")
	TObjectPtr<UTileMapDataAsset> StageData;


	UPROPERTY()
	TObjectPtr<AGridVisualizer> GridVisualizer;
};

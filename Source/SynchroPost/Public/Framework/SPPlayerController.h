#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SPPlayerController.generated.h"

class AGridVisualizer;
class UTileMapDataAsset;
class AUnit;
class UNodeSelectionWidget;

UCLASS()
class SYNCHROPOST_API ASPPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	
public:
	ASPPlayerController();
	virtual void BeginPlay() override;


	UFUNCTION(BlueprintCallable, Category = "Move Mode")
	void EnterMoveMode();

	UFUNCTION(BlueprintCallable, Category = "Move Mode")
	void ExitMoveMode();

	UFUNCTION(BlueprintCallable, Category = "Move Mode")
	void ConfirmMove();


	UFUNCTION(Client, Reliable)
	void Client_ShowNodeSelection();

	UFUNCTION(Server, Reliable)
	void Server_RequestEnterNode(int32 NodeIndex);

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void DebugKillHoveredUnit();


	FORCEINLINE AGridVisualizer* GetGridVisualizer() const { return GridVisualizer; }
	
protected:

	virtual void Tick(float DeltaSeconds) override;

	void UpdateHoverTile();
protected:

	UPROPERTY(EditDefaultsOnly, Category = "Grid Visualizer")
	TSubclassOf<AGridVisualizer> GridVisualizerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Grid")
	TObjectPtr<UTileMapDataAsset> StageData;

	UPROPERTY()
	TObjectPtr<AGridVisualizer> GridVisualizer;

	FVector2D LastMouseScreenPosition = FVector2D(-1.0f, -1.0f);

	UPROPERTY()
	bool bIsInMoveMode = false;

	UPROPERTY()
	TObjectPtr<AUnit> MovingUnit;

	UPROPERTY()
	TArray<FIntPoint> CachedReachableCoords;

	FIntPoint LastHoveredCoord = FIntPoint(MIN_int32, MIN_int32);

	UPROPERTY()
	TArray<FIntPoint> CurrentPathCoords;


	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNodeSelectionWidget> NodeSelectionWidgetClass;

	UPROPERTY()
	TObjectPtr<UNodeSelectionWidget> NodeSelectionWidgetInstance;
};

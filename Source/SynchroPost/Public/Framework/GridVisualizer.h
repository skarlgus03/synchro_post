#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridVisualizer.generated.h"

class UInstancedStaticMeshComponent;
class UGridManager;

UENUM(BlueprintType)
enum class ETileVisualState : uint8
{
	Default,
	InRange,
	OnPath,
	Hovered,
	ValidTarget
};

USTRUCT()
struct FTileStateList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<ETileVisualState> States;

};


UCLASS()
class SYNCHROPOST_API AGridVisualizer : public AActor
{
	GENERATED_BODY()

public:
	AGridVisualizer();

	// 그리드 전체를 BaseGridMesh에 채워 넣는다
	UFUNCTION(BlueprintCallable, Category = "Grid Visualizer")
	void PopulateFromGrid();

	
	UFUNCTION(BlueprintCallable, Category = "Grid Visualizer")
	void AddTileState(const FIntPoint& Coord, ETileVisualState State);

	UFUNCTION(BlueprintCallable, Category = "Grid Visualizer")
	void RemoveTileState(const FIntPoint& Coord, ETileVisualState State);

	UFUNCTION(BlueprintCallable, Category = "Grid Visualizer")
	void AddTileStates(const TArray<FIntPoint>& Coords, ETileVisualState State);

	UFUNCTION(BlueprintCallable, Category = "Grid Visualizer")
	void RemoveTileStates(const TArray<FIntPoint>& Coords, ETileVisualState State);

private:
	void RefreshTileVisual(const FIntPoint& Coord);
	FLinearColor ResolveColor(ETileVisualState State) const;

protected:


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UInstancedStaticMeshComponent> GridMesh;

	UPROPERTY()
	TMap<FIntPoint, int32> CoordToInstanceIndex;

	UPROPERTY()
	TObjectPtr<UGridManager> CachedGridManager;



	UPROPERTY(EditAnywhere, Category = "Grid Visualizer")
	float HeightOffset = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Grid Visualizer")
	float TileScale = 0.9f;


	// 각 좌표에 대한 시각적 상태를 저장하는 맵. 여러 상태를 동시에 적용할 수 있도록 TArray를 사용
	UPROPERTY()
	TMap<FIntPoint, FTileStateList> ActiveTileStates; 

	UPROPERTY(EditDefaultsOnly, Category = "Colors")
	TMap<ETileVisualState, FLinearColor> StateColors;
};
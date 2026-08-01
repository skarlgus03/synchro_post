#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridVisualizer.generated.h"

class UInstancedStaticMeshComponent;
class UGridManager;

UCLASS()
class SYNCHROPOST_API AGridVisualizer : public AActor
{
	GENERATED_BODY()

public:
	AGridVisualizer();

	// 그리드 전체를 BaseGridMesh에 채워 넣는다
	UFUNCTION(BlueprintCallable, Category = "Grid Visualizer")
	void PopulateFromGrid();

	// BaseGridMesh, HighlightMesh 인스턴스를 모두 지운다
	UFUNCTION(BlueprintCallable, Category = "Grid Visualizer")
	void ClearAll();

	// 지정된 좌표들만 하이라이트로 표시 (기존 하이라이트는 지우고 새로 그림)
	UFUNCTION(BlueprintCallable, Category = "Grid Visualizer")
	void ShowHighlightedTiles(const TArray<FIntPoint>& Coords);

	UFUNCTION(BlueprintCallable, Category = "Grid Visualizer")
	void ClearHighlightedTiles();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UInstancedStaticMeshComponent> BaseGridMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UInstancedStaticMeshComponent> HighlightMesh;

	UPROPERTY()
	TObjectPtr<UGridManager> CachedGridManager;

	UPROPERTY(EditAnywhere, Category = "Grid Visualizer")
	float BaseHeightOffset = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Grid Visualizer")
	float HighlightHeightOffset = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Grid Visualizer")
	float TileScale = 0.9f;
};
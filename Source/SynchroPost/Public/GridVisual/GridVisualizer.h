#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridVisualizer.generated.h"

class UInstancedStaticMeshComponent;
class UGridManager;
class UGridOutlineComponent;
class UGridPathComponent;



// 액션 모드가 채우고 GridVisualizer 가 그리는 지금 보여줄 것.
// 매번 통째로 만들어 넘김.
struct FGridPreview
{
	TArray<FIntPoint> RangeTiles;		// 이동 가능한 범위
	TArray<FIntPoint> PathTiles;		// 이동 경로
	TArray<FIntPoint> ImpactTiles;		// 영향 범위
	TOptional<FIntPoint> TargetTile;	// 목표 타일
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
		
	// 미리보기를 통째로 교체한다. 이전 미리보기는 여기서 지운다.
	void ApplyPreview(const FGridPreview& Preview);
	void ClearPreview();

private:
	
	FGridPreview CurrentPreview;

	TArray<FIntPoint> PathTiles;		// 이동 경로 (시작 칸 포함)

protected:


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UInstancedStaticMeshComponent> GridMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UGridOutlineComponent> Outline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UGridPathComponent> PathArrow;

	UPROPERTY()
	TObjectPtr<UGridManager> CachedGridManager;



	UPROPERTY(EditAnywhere, Category = "Grid Visualizer")
	float HeightOffset = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Grid Visualizer")
	float TileScale = 0.9f;

};
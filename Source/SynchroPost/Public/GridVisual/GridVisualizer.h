#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridVisualizer.generated.h"

class UInstancedStaticMeshComponent;
class UGridManager;
class UGridOutlineComponent;
class UGridPathComponent;

class UMaterialParameterCollection;
class UGridTileOverlayComponent;

class AUnit;
class UGridGhostComponent;


// 목적지에 설 유닛 미리보기
struct FGridGhost
{
	TWeakObjectPtr<AUnit> Unit; 	// 미리보기할 유닛
	FIntPoint Coord;				// 어디에 설지
	FIntPoint FacingFrom;			// 어디 칸에서 들어오나 (도착 방향)
};

// 액션 모드가 채우고 GridVisualizer 가 그리는 지금 보여줄 것.
// 매번 통째로 만들어 넘김.
struct FGridPreview
{
	TArray<FIntPoint> RangeTiles;		// 이동 가능한 범위
	TArray<FIntPoint> PathTiles;		// 이동 경로
	TArray<FIntPoint> ImpactTiles;		// 영향 범위
	TOptional<FIntPoint> TargetTile;	// 목표 타일
	TOptional<FGridGhost> Ghost;		// 이동 목적지에 설 유닛 미리보기
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

	// 격자를 드러내거나 숨긴다. (RevealFadeDuration 동안 서서히 Fade)
	void SetGridRevealed(bool bRevealed);

private:
	
	void ApplyReveal(float Value);

	// 실제로 미리보기 메시를 지움.
	void ClearPreviewMeshes();

	float GetFadeDuration() const
	{
		return (TargetReveal > CurrentReveal) ? RevealFadeInDuration : RevealFadeOutDuration;
	}

	float CurrentReveal = 0.f;
	float TargetReveal = 0.f;
	
	// 격자가 다 사라지면 메시를 지운다. (FadeOut 중에 메시가 보이면 안 됨)
	bool bClearWhenHidden = false;

	FGridPreview CurrentPreview;
	TArray<FIntPoint> PathTiles;		// 이동 경로 (시작 칸 포함)

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;



	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UInstancedStaticMeshComponent> GridMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UGridTileOverlayComponent> TileOverlay;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UGridOutlineComponent> Outline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UGridPathComponent> PathArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Visualizer")
	TObjectPtr<UGridGhostComponent> GhostMesh;

	UPROPERTY()
	TObjectPtr<UGridManager> CachedGridManager;

	UPROPERTY(EditAnywhere, Category = "Grid Visualizer")
	TObjectPtr<UMaterialParameterCollection> RevealCollection;

	UPROPERTY(EditAnywhere, Category = "Grid Visualizer")
	float HeightOffset = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Grid Visualizer")
	float TileScale = 0.9f;
	
	// 격자가 나타나는 시간
	UPROPERTY(EditAnywhere, Category = "Grid Visualizer", meta = (ClampMin = "0.0"))
	float RevealFadeInDuration = 0.15f;

	// 격자가 사라지는 시간
	UPROPERTY(EditAnywhere, Category = "Grid Visualizer", meta = (ClampMin = "0.0"))
	float RevealFadeOutDuration = 0.15f;
};
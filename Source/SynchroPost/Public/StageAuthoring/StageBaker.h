#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageBaker.generated.h"

class UStageDataAsset;
class UInstancedStaticMeshComponent;
class UBillboardComponent;
class UMaterialInterface;

/**
 * 레벨 지형(+ 나중에 마커) → StageDA 굽기 도구. 레벨당 1개, 에디터 전용.
 * 액터 위치 = 그리드 원점(타일 (0,0)의 중심). 회전은 무시한다(축 정렬).
 * 구운 DA는 손으로 고치지 않는다. 레벨에서 고쳐 다시 굽는다.
 */
UCLASS(HideCategories = (Rendering, Physics, Collision, Input, LOD, Cooking, Replication, HLOD, Networking))
class SYNCHROPOST_API AStageBaker : public AActor
{
	GENERATED_BODY()

public:
	AStageBaker();

	virtual void OnConstruction(const FTransform& Transform) override;

	// Target(DA)에 이 레벨의 데이터를 굽는다
	UFUNCTION(CallInEditor, Category = "Stage Baker")
	void Bake();

	// 지형(벽 메시 등)을 옮긴 뒤 미리보기 갱신. 베이커 자신이 바뀔 땐 OnConstruction이 알아서 다시 그림
	UFUNCTION(CallInEditor, Category = "Stage Baker")
	void RefreshPreview();

protected:
	// 굽기 대상 StageDA. CombatStageDA면 그리드 옵션 표시
	UPROPERTY(EditAnywhere, Category = "Stage Baker")
	TObjectPtr<UStageDataAsset> Target; 

	// Target의 클래스로 계산. 아래 그리드 옵션의 표시 조건. CombatStageDA면 true
	UPROPERTY(Transient)
	bool bTargetIsCombat = false;

	UPROPERTY(EditAnywhere, Category = "Stage Baker|Grid", meta = (EditCondition = "bTargetIsCombat", EditConditionHides, ClampMin = "1", ClampMax = "100"))
	int32 GridWidth = 10;

	UPROPERTY(EditAnywhere, Category = "Stage Baker|Grid", meta = (EditCondition = "bTargetIsCombat", EditConditionHides, ClampMin = "1", ClampMax = "100"))
	int32 GridHeight = 10;

	UPROPERTY(EditAnywhere, Category = "Stage Baker|Grid", meta = (EditCondition = "bTargetIsCombat", EditConditionHides, ClampMin = "1"))
	float TileSize = 180.0f;

	// 원점 Z + 이 값보다 높은 곳에 맞으면 벽. 아무것도 안 맞으면(구멍) 역시 벽
	UPROPERTY(EditAnywhere, Category = "Stage Baker|Grid", meta = (EditCondition = "bTargetIsCombat", EditConditionHides, ClampMin = "0"))
	float FloorTolerance = 15.0f;

	// 원점 Z 기준 이 높이에서 아래로 쏜다. 이 높이가 지형 안이면 벽. 천장은 이보다 위라서 무시됨
	// WallHeightThreshold보다 커야 함
	UPROPERTY(EditAnywhere, Category = "Stage Baker|Grid", meta = (EditCondition = "bTargetIsCombat", EditConditionHides, ClampMin = "1"))
	float ClearanceHeight = 150.0f;

	// 원점 Z 아래로 바닥을 찾을 거리. 이보다 깊으면 구멍
	UPROPERTY(EditAnywhere, Category = "Stage Baker|Grid", meta = (EditCondition = "bTargetIsCombat", EditConditionHides, ClampMin = "1"))
	float TraceDepth = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Stage Baker|Preview", meta = (EditCondition = "bTargetIsCombat", EditConditionHides))
	TSoftObjectPtr<UMaterialInterface> PreviewMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/SynchroPost/SP_Test_M.SP_Test_M")));

	// 칸 대비 판 크기 (틈으로 칸 경계를 보이게)
	UPROPERTY(EditAnywhere, Category = "Stage Baker|Preview", meta = (EditCondition = "bTargetIsCombat", EditConditionHides, ClampMin = "0.1", ClampMax = "1.0"))
	float PreviewTileScale = 0.92f;

	UPROPERTY(EditAnywhere, Category = "Stage Baker|Preview", meta = (EditCondition = "bTargetIsCombat", EditConditionHides))
	float PreviewHeightOffset = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Stage Baker|Preview", meta = (EditCondition = "bTargetIsCombat", EditConditionHides))
	FLinearColor FloorColor = FLinearColor(0.1f, 0.8f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Stage Baker|Preview", meta = (EditCondition = "bTargetIsCombat", EditConditionHides))
	FLinearColor WallColor = FLinearColor(0.9f, 0.1f, 0.1f, 1.0f);

	// 칸마다 레이저를 뷰포트에 그림. 미리보기를 다시 그릴 때마다 갱신
	UPROPERTY(EditAnywhere, Category = "Stage Baker|Preview", meta = (EditCondition = "bTargetIsCombat", EditConditionHides))
	bool bDrawProbeDebug = false;

private:
	UPROPERTY(VisibleAnywhere, Category = "Stage Baker")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> PreviewMesh;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif

	// FTileGrid::CoordToWorld와 같은 공식. 원점 = 액터 위치
	FVector CoordToWorld(const FIntPoint& Coord) const;

	// 칸 중심에서 위→아래 트레이스. 걸을 수 있는 바닥이면 true. OutSurface = 맞은 지점(구멍이면 칸 중심)
	bool ProbeTile(const FIntPoint& Coord, FVector& OutSurface) const;

	void RebuildPreview();
};
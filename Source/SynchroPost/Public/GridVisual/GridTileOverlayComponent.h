#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayTagContainer.h"
#include "GridTileOverlayComponent.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

USTRUCT()
struct FTileOverlayStyle
{
	GENERATED_BODY()

	// 칸 하나에 놓일 모양. XY를 칸 크기에 맞게 늘린다 (평평한 막 = Plane, 홀로그램 = 위아래 뚫린 상자)
	UPROPERTY(EditAnywhere, Category = "Overlay")
	TObjectPtr<UStaticMesh> Mesh;

	// GridReveal(MPC)을 곱해야 격자와 함께 나타나고 사라진다
	UPROPERTY(EditAnywhere, Category = "Overlay")
	TObjectPtr<UMaterialInterface> Material;

	// 칸 크기 대비 비율. 1보다 작으면 이웃 칸과 틈이 생긴다
	UPROPERTY(EditAnywhere, Category = "Overlay", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float TileScale = 0.9f;

	// 바닥에서 뜬 높이 (cm). 층마다 다르게 해 겹쳐도 깜빡이지 않게
	UPROPERTY(EditAnywhere, Category = "Overlay")
	float Height = 2.0f;

	// 반투명 정렬 순서. 외곽선(10), 경로(11)보다 낮게
	UPROPERTY(EditAnywhere, Category = "Overlay")
	int32 SortPriority = 5;
};

/**
 * 칸 위에 덮는 타일들. 태그(의미)마다 ISM 하나 = 메시 하나 + 머티리얼 하나.
 * 처음 쓰는 태그면 Styles를 보고 ISM을 만들어 붙인다.
 * 새 표시 추가 = 태그 + 메시 + MI (코드 수정 없음)
 * 로컬 연출 전용 (복제 안 함, 콜리전 없음)
 */
UCLASS()
class SYNCHROPOST_API UGridTileOverlayComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGridTileOverlayComponent();

	// 그 태그의 타일들을 이 칸들로 교체한다. 비어 있으면 지운다
	void SetLayer(const FGameplayTag& Tag, const TArray<FIntPoint>& Tiles);
	void ClearLayer(const FGameplayTag& Tag);
	void ClearAllLayers();

protected:
	// 태그마다 메시와 머티리얼을 지정한다. 없으면 표시 못 한다
	UPROPERTY(EditAnywhere, Category = "Overlay", meta = (Categories = "Grid.Overlay"))
	TMap<FGameplayTag, FTileOverlayStyle> Styles;

private:
	// 그 태그의 ISM. 없으면 스타일을 보고 만든다. 스타일이나 메시가 없으면 nullptr
	UInstancedStaticMeshComponent* GetOrCreateLayer(const FGameplayTag& Tag);

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UInstancedStaticMeshComponent>> Layers;
};
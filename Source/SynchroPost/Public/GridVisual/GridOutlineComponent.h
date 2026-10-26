#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GridOutlineComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

// 외곽선 층. 값 = 메시 섹션 번호
UENUM()
enum class EGridOutlineLayer : uint8
{
	Range,	// 이동 범위/사거리
	Impact, // 스킬 효과 범위
	Target, // 커서가 가리키는 유효한 칸
	Count UMETA(Hidden)
};

USTRUCT()
struct FGridOutlineStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Outline")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Outline", meta = (ClampMin = "0"))
	float Intensity = 2.0f;

	// 띠의 폭 (cm) 대역폭아님
	UPROPERTY(EditAnywhere, Category = "Outline", meta = (ClampMin = "0.1"))
	float BandWidth = 8.0f;

	// 칸 경계에서 안쪽으로 들어온 거리 (cm). 옆 범위의 띠와 겹치지 않게
	UPROPERTY(EditAnywhere, Category = "Outline", meta = (ClampMin = "0"))
	float Inset = 4.0f;

	// 바닥에서 뜬 높이 (cm). 층마다 다르게 주면 겹쳐도 깜빡이지 않는다
	UPROPERTY(EditAnywhere, Category = "Outline")
	float FloatHeight = 3.0f;
};

/**
 * 칸 집합을 외곽선 띠(바닥에서 살짝 뜬 납작한 리본)로 그린다. 층마다 메시 섹션 하나.
 * 칸 목록 → GridOutline::BuildLoops(고리) → 고리마다 액자 모양 띠 → 그 층의 섹션 재생성
 * 로컬 연출 전용 (복제 안 함, 콜리전 없음)
 */
UCLASS()
class SYNCHROPOST_API UGridOutlineComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()
	
public:
	UGridOutlineComponent(const FObjectInitializer& ObjectInitializer);

	// 그 층을 이 칸들의 외곽선으로 교체한다. 비어 있으면 지운다
	void SetLayer(EGridOutlineLayer Layer, const TArray<FIntPoint>& Tiles);
	void ClearLayer(EGridOutlineLayer Layer);
	void ClearAllLayers();

protected:

	// 층 공용 머티리얼 (Unlit, Translucent, Two Sided, 파라미터 Color / Intensity)
	UPROPERTY(EditAnywhere, Category = "Outline")
	TObjectPtr<UMaterialInterface> OutlineMaterial;

	UPROPERTY(EditAnywhere, Category = "Outline")
	TMap<EGridOutlineLayer, FGridOutlineStyle> Styles;

private:
	const FGridOutlineStyle& GetStyle(EGridOutlineLayer Layer) const;

	// 층마다 머티리얼 인스턴스 하나 (처음 쓸 때 만든다)
	UMaterialInstanceDynamic* GetLayerMaterial(EGridOutlineLayer Layer);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> LayerMaterials;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GridPathComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 월드 꺾은선을 바닥에서 살짝 뜬 화살표 띠로 그린다. 섹션 하나.
 * 꺾은선 → 앞(StartGap)·뒤(HeadLength) 잘라내기 → 몸통 띠 + 삼각형 화살촉
 * Grid를 모른다: 점만 주면 그린다 (이동 경로, 나중에 AI 의도 표시 등)
 * 로컬 연출 전용 (복제 안 함, 콜리전 없음)
 */
UCLASS()
class SYNCHROPOST_API UGridPathComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()
	
public:
	UGridPathComponent(const FObjectInitializer& ObjectInitializer);

	// 이 꺾은선으로 화살표를 교체한다. 점이 2개 미만이면 지움
	void SetPath(const TArray<FVector>& WorldPoints);
	void ClearPath();

protected:

	// Unlit, Translucent, Two Sided. 파라미터 Color / Intensity
	// UV: U = 출발점부터 거리 / UVScale, V = 0 왼쪽 → 1 오른쪽
	UPROPERTY(EditAnywhere, Category = "Path")
	TObjectPtr<UMaterialInterface> PathMaterial;

	// 몸통 폭 (cm)
	UPROPERTY(EditAnywhere, Category = "Path", meta = (ClampMin = "1"))
	float Width = 14.0f;

	// 화살촉 길이 (cm)
	UPROPERTY(EditAnywhere, Category = "Path", meta = (ClampMin = "1"))
	float HeadLength = 30.0f;

	// 화살촉 폭 (cm)
	UPROPERTY(EditAnywhere, Category = "Path", meta = (ClampMin = "1"))
	float HeadWidth = 36.0f;

	// 출발점 쪽을 잘라낼 길이 (cm). 유닛 발밑에 가려지는 부분
	UPROPERTY(EditAnywhere, Category = "Path", meta = (ClampMin = "0"))
	float StartGap = 30.0f;

	// 바닥에서 뜬 높이 (cm). 외곽선 층들보다 위로
	UPROPERTY(EditAnywhere, Category = "Path")
	float FloatHeight = 6.0f;

	// U 1당 거리 (cm). 타일 크기와 같게 두면 U가 칸 단위
	UPROPERTY(EditAnywhere, Category = "Path", meta = (ClampMin = "1"))
	float UVScale = 100.0f;

};

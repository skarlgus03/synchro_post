

#pragma once

#include "CoreMinimal.h"

class UGridManager;
class UWorld;

/*
* 칸 집합의 외곽선 (고리) 계산. 순수 격자 계싼이라 그리는 방식과 무관함.
* 
* 꼭짓점 좌표계 : 칸 (x,y) 의 네 모서리 = (x,y), (x+1,y), (x+1,y+1), (x,y+1)
* 고리 = 꼭짓점 목록 (닫힘. 첫 점을 끝에 반복하지않음). 직선 위 중간 꼭짓점은 뺌. (2x2 블록은 4개)
* 방향 : 영역이 항상 진행 방향의 왼쪽. (격자가 x 오른쪽, y 위 기준)
*  -> 바깥 고리는 넓이 + , 구멍 고리는 넓이 - , 띠를 만들 떄 안쪽 = 진행 방향의 왼쪽
* 대각선으로만 맞닿은 칸은 서로 다른 고리로 취급한다.
*/
namespace GridOutline
{
	SYNCHROPOST_API TArray<TArray<FIntPoint>> BuildLoops(const TArray<FIntPoint>& Tiles);

	// 신발끈 공식 넓이 (칸 1개 = 1). 바깥 고리 +, 구멍 고리 −
	SYNCHROPOST_API double SignedArea(const TArray<FIntPoint>& Loop);

	// 꼭짓점 → 월드 위치 (칸 중심에서 반 칸씩 어긋난 자리)
	// 그리드가 월드 축에 정렬되어 있다고 가정한다. (그리드 회전 미지원)
	SYNCHROPOST_API FVector CornerToWorld(const UGridManager& Grid, const FIntPoint& Corner);

	// 디버그: 고리를 바닥에 그린다. 바깥 = 초록, 구멍 = 주황, 화살표 = 진행 방향
	SYNCHROPOST_API bool IsDebugEnabled();
	SYNCHROPOST_API void DrawDebugLoops(const UWorld* World, const TArray<TArray<FIntPoint>>& Loops, float Duration);
}
#pragma once

#include "CoreMinimal.h"

class UGridManager;

/*
 * 격자 경로(칸 목록) → 부드러운 월드 꺾은선.
 * 유닛 이동 연출과 경로 화살표가 같은 함수를 써서 "보여준 경로 = 실제로 걷는 경로"가 되게 한다.
 */
namespace GridPath
{
	/**
	 * 실 당기기 → 일직선 합치기 → 대각선 1/4칸 옮기기 → 모서리 둥글리기
	 *
	 * @param Waypoints     시작 칸 포함, 이어진 칸 목록. 2개 미만이면 빈 배열
	 * @param ToWorld       칸 → 월드 위치로 바꾸는 함수. XY는 칸 중심이어야 하고 높이만 호출 쪽이 정한다
	 * @param StartLocation 있으면 첫 점을 이 위치로 바꾼다 (유닛의 실제 현재 위치)
	 * @param OutCorners    디버그용: 실 당기기 + 일직선 합치기 결과 칸
	 */
	SYNCHROPOST_API TArray<FVector> BuildSmoothPath(
		const UGridManager& Grid,
		const TArray<FIntPoint>& Waypoints,
		TFunctionRef<FVector(const FIntPoint&)> ToWorld,
		const TOptional<FVector>& StartLocation = TOptional<FVector>(),
		TArray<FIntPoint>* OutCorners = nullptr);
}
#pragma once

#include "CoreMinimal.h"
#include "DataBridgeTypes.h"

// 두 IR 값을 비교해 바뀐 곳을 사람이 읽는 문장으로 모은다.
// 예: "assets[0].data.HealthBarZOffset: 20 -> 50"
namespace DataBridgeDiff
{
	// Before와 After를 비교해 바뀐 곳을 OutChanges에 모은다. 숫자는 허용 오차로 비교한다.
	void Collect(const TSharedPtr<FDataBridgeValue>& Before, const TSharedPtr<FDataBridgeValue>& After,
		const FString& Path, TArray<FString>& OutChanges, double Tolerance = 1e-4);

	// 짧은 표시용 문자열 (숫자, "문자열", true, null, [3 items], {...})
	FString Describe(const TSharedPtr<FDataBridgeValue>& Value);
}
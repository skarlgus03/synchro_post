#pragma once

#include "CoreMinimal.h"
#include "DataBridgeTypes.h"

struct FDataBridgeImportPlan;

// 고정 파이프라인. 확장 지점이 아니다.
class DATABRIDGE_API FDataBridgePipeline
{
public:
	// IR 봉투 → 실행 계획. 에셋을 변경하지 않는다(읽기만).
	// 에러를 끝까지 모아서 Ctx에 남기고, 하나라도 있으면 false (OutPlan은 비움).
	static bool BuildImportPlan(const FDataBridgeObject& Root, FDataBridgeImportPlan& OutPlan, FDataBridgeContext& Ctx);
};
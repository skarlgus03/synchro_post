#pragma once

#include "CoreMinimal.h"
#include "DataBridgeTypes.h"

class FDataBridgeReferenceResolver;

struct FDataBridgeImportPlan;

// 고정 파이프라인. 확장 지점이 아니다.
class DATABRIDGE_API FDataBridgePipeline
{
public:
	// IR 봉투 → 실행 계획. 에셋을 변경하지 않는다(읽기만).
	// 에러를 끝까지 모아서 Ctx에 남기고, 하나라도 있으면 false (OutPlan은 비움).
	static bool BuildImportPlan(const FDataBridgeObject& Root, FDataBridgeImportPlan& OutPlan, FDataBridgeContext& Ctx);

	// 에셋 목록 → IR 봉투. 참조는 실제 경로로 쓴다. 에러가 있으면 nullptr.
	static TSharedPtr<FDataBridgeObject> Export(TConstArrayView<UObject*> Assets, FDataBridgeContext& Ctx);

	// 주소록을 지정하는 버전 (Dry-run diff에서 복제본을 원래 경로로 내보낼 때)
	static TSharedPtr<FDataBridgeObject> Export(TConstArrayView<UObject*> Assets, const FDataBridgeReferenceResolver& Resolver, FDataBridgeContext& Ctx);

	// 연습 (= Dry-run): 계획을 임시 복제본에 적용하고 검증한다. 진짜 에셋은 바꾸지 않는다.
	// OutChanges가 있으면 "assets[i].data.필드: 전 -> 후" 형식의 변경 목록을 채운다.
	// 에러가 하나도 없으면 true
	static bool Rehearse(const FDataBridgeImportPlan& Plan, FDataBridgeContext& Ctx, TArray<FString>* OutChanges = nullptr);
};
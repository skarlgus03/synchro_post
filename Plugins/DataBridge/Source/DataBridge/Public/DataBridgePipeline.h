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


	// 실전: 계획을 진짜 에셋에 적용한다. 하나의 Undo 기록("DataBridge Import")으로 묶는다.
	// 실패하면 기존 에셋은 되돌리고 새로 만든 에셋은 지운다. 저장은 하지 않는다(dirty 상태로 둔다).
	// 새로 만든 에셋은 Undo 대상이 아니다 (에디터의 에셋 생성과 같은 규칙).
	// Rehearse를 통과한 계획을 넘길 것
	static bool Commit(const FDataBridgeImportPlan& Plan, FDataBridgeContext& Ctx);

	// 계획 → 연습 → 실전. 연습이 실패하면 실전은 하지 않는다
	static bool Import(const FDataBridgeObject& Root, FDataBridgeContext& Ctx,
		FDataBridgeImportPlan* OutPlan = nullptr, TArray<FString>* OutChanges = nullptr);

	// 클래스 → 설명서: 필드, 타입, 허용 값, 기본값(defaults), 주석 없는 필드 목록.
	// AI가 이 클래스의 data를 쓸 때 참고하는 문서. 에셋을 바꾸지 않는다. 에러가 있으면 nullptr
	static TSharedPtr<FDataBridgeObject> Describe(const UClass* Class, FDataBridgeContext& Ctx);

#if WITH_DEV_AUTOMATION_TESTS
	// 테스트 전용: true면 다음 Commit이 적용을 끝낸 직후 일부러 실패한다 (롤백 검증용). 한 번 쓰이면 false로 돌아간다
	static bool bFailNextCommitForTest;
#endif
};
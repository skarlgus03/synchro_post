#pragma once

#include "CoreMinimal.h"
#include "DataBridgeTypes.h"

class FDataBridgeSchemaBuilder;
class FMapProperty;
class FObjectPropertyBase;
class FProperty;
class UClass;
class UEnum;
class UScriptStruct;
class UStruct;

// 워커 (Describe 방향): 리플렉션으로 클래스의 편집 가능한 필드를 "모양"으로 펼친다. 값은 보지 않는다.
// Export/Import 워커와 같은 규칙(ShouldWalk, 포매터, 비트마스크, 맵 형태, Instanced)을 따라야 한다 (왕복 계약의 문서판).
// 만난 구조체와 Instanced 클래스는 Schema의 정의 테이블에 한 번씩 올린다.
// Describe 한 번 동안만 쓰는 일회용 객체.
class FDataBridgeDescribeWalker
{
public:
	FDataBridgeDescribeWalker(FDataBridgeSchemaBuilder& InSchema, FDataBridgeContext& InCtx);

	// 클래스를 설명서에 올린다 (이미 올라가 있으면 아무것도 안 함)
	void DescribeClass(const UClass* Class);

private:
	// { "필드": 타입 설명, ... }
	TSharedRef<FDataBridgeObject> DescribeFields(const UStruct* Struct);

	// 필드 하나: 타입 + 메타(범위, 태그 접두사) + 고정 배열. 지원하지 않는 타입이면 nullptr
	TSharedPtr<FDataBridgeObject> DescribeProperty(const FProperty* Property);

	// 타입별 분기 (ExportValue와 같은 순서)
	TSharedPtr<FDataBridgeObject> DescribeValue(const FProperty* Property);

	TSharedRef<FDataBridgeObject> DescribeStruct(const UScriptStruct* Struct);
	TSharedPtr<FDataBridgeObject> DescribeMap(const FMapProperty* Property);
	TSharedPtr<FDataBridgeObject> DescribeObjectReference(const FObjectPropertyBase* Property);
	TSharedRef<FDataBridgeObject> DescribeInstanced(const UClass* BaseClass);
	TSharedRef<FDataBridgeObject> DescribeEnum(const UEnum* Enum);

	FDataBridgeSchemaBuilder& Schema;
	FDataBridgeContext& Ctx;

	TSet<const UClass*> WarnedEmptyInstancedBases; // 이미 경고한 Instanced 클래스는 한 번만 경고한다
};
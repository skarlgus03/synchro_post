#pragma once

#include "CoreMinimal.h"
#include "DataBridgeTypes.h"

class FDataBridgeReferenceResolver;
class FMapProperty;
class FObjectPropertyBase;
class FProperty;
class UEnum;
class UScriptStruct;
class UStruct;

// 워커 (Export 방향): 리플렉션으로 오브젝트의 편집 가능한 필드를 IR로 펼친다. 읽기만 한다.
// Export 한 번 동안만 쓰는 일회용 객체.
class FDataBridgeExportWalker
{
public:
	FDataBridgeExportWalker(const FDataBridgeReferenceResolver& InResolver, FDataBridgeContext& InCtx);

	// 오브젝트 → { "필드": 값, ... }
	TSharedPtr<FDataBridgeObject> ExportObject(const UObject* Object);

	// 워커가 다루는 필드인가. EditAnywhere만, Transient/Deprecated/VisibleAnywhere 제외.
	// Import 워커도 같은 규칙을 쓴다 (왕복 계약).
	static bool ShouldWalk(const FProperty* Property);

private:
	// 필드 목록 돌기. 리플렉션으로 필드를 하나씩 꺼내서 Json객체에 넣음
	void ExportFieldsInto(FDataBridgeObject& Out, const UStruct* Struct, const void* Container);
	
	// 타입별 분기 함수들

	TSharedPtr<FDataBridgeValue> ExportProperty(const FProperty* Property, const void* ValuePtr);
	TSharedPtr<FDataBridgeValue> ExportValue(const FProperty* Property, const void* ValuePtr);
	TSharedPtr<FDataBridgeValue> ExportStruct(const UScriptStruct* Struct, const void* ValuePtr);
	TSharedPtr<FDataBridgeValue> ExportMap(const FMapProperty* MapProperty, const void* ValuePtr);
	TSharedPtr<FDataBridgeValue> ExportObjectReference(const FObjectPropertyBase* Property, const void* ValuePtr);
	TSharedPtr<FDataBridgeValue> ExportInstanced(const UObject* Object);
	TSharedPtr<FDataBridgeValue> ExportEnumValue(const UEnum* Enum, int64 Value);

	const FDataBridgeReferenceResolver& Resolver;
	FDataBridgeContext& Ctx;
	int32 InstancedDepth = 0;
};
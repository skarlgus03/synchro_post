#pragma once

#include "CoreMinimal.h"
#include "DataBridgeTypes.h"

class FArrayProperty;
class FDataBridgeReferenceResolver;
class FMapProperty;
class FNumericProperty;
class FObjectPropertyBase;
class FProperty;
class FSetProperty;
class FTextProperty;
class UEnum;
class UScriptStruct;
class UStruct;

// Walker (Import 방향): IR을 따라 걸으며 오브젝트의 필드에 값을 넣는다.
// - JSON에 있는 키만 바꾼다 (생략 = 기존 값 유지). 모르는 키, 편집 불가 필드는 에러.
// - 배열/Set/맵은 통째로 교체한다.
// - Instanced 블록은 항상 새 오브젝트를 만들어 채운다 (생략한 필드 = 클래스 기본값).
// Import 한 번 동안만 쓰는 일회용 객체.
class FDataBridgeImportWalker
{
public:
	FDataBridgeImportWalker(const FDataBridgeReferenceResolver& InResolver, FDataBridgeContext& InCtx);

	// data 객체 → Target. 이번 호출에서 에러가 하나도 없으면 true
	bool ImportObject(const FDataBridgeObject& Data, UObject* Target);

	// 이번 Import로 새로 만든 Instanced 오브젝트들 (⑧에서 실패 시 정리용)
	const TArray<UObject*>& GetCreatedObjects() const { return CreatedObjects; }

private:
	void ImportFields(const FDataBridgeObject& Json, const UStruct* Struct, void* Container, bool bSkipInstancedClassKey);
	bool ImportProperty(const FProperty* Property, const FDataBridgeValue& Value, void* ValuePtr);
	bool ImportValue(const FProperty* Property, const FDataBridgeValue& Value, void* ValuePtr);
	bool ImportNumeric(const FNumericProperty* Property, const FDataBridgeValue& Value, void* ValuePtr);
	bool ImportEnum(const UEnum* Enum, const FDataBridgeValue& Value, int64& OutValue);
	bool ImportText(const FTextProperty* Property, const FDataBridgeValue& Value, void* ValuePtr);
	bool ImportStruct(const UScriptStruct* Struct, const FDataBridgeValue& Value, void* ValuePtr);
	bool ImportArray(const FArrayProperty* Property, const FDataBridgeValue& Value, void* ValuePtr);
	bool ImportSet(const FSetProperty* Property, const FDataBridgeValue& Value, void* ValuePtr);
	bool ImportMap(const FMapProperty* Property, const FDataBridgeValue& Value, void* ValuePtr);
	bool ImportObjectReference(const FObjectPropertyBase* Property, const FDataBridgeValue& Value, void* ValuePtr);
	bool ImportInstanced(const FObjectPropertyBase* Property, const FDataBridgeValue& Value, void* ValuePtr);

	const FDataBridgeReferenceResolver& Resolver;
	FDataBridgeContext& Ctx;
	UObject* CurrentOuter = nullptr;   // 새 Instanced 오브젝트의 주인 (에셋 또는 바깥 Instanced)
	int32 InstancedDepth = 0;
	TArray<UObject*> CreatedObjects;
};

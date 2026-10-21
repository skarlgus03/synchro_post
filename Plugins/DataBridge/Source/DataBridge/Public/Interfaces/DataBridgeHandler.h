#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"
#include "DataBridgeTypes.h"

class FDataBridgeContext;
class UClass;
class UObject;
class FDataBridgeReferenceResolver;
class FDataBridgeSchemaBuilder;

// IR(봉투의 data 객체) ⇄ 에셋. 파일 포맷에 대해서는 아무것도 모른다.
// ex) UDataTable, UCurveFloat 등 각 에셋별로 구현한다.
// 핸들러를 갈아 끼울 수 있게 한 이유는, 필드를 그냥 나열하면 엉망이되는 에셋이 있음
// ex) Material Instance
class IDataBridgeHandler : public IModularFeature
{
public:
	static FName GetModularFeatureName()
	{
		static const FName Name(TEXT("DataBridgeHandler"));
		return Name;
	}

	virtual ~IDataBridgeHandler() = default;

	// 처리 가능한 최상위 클래스.
	// 창구는 대상 에셋 클래스와 상속 거리가 가장 가까운 핸들러를 고른다.
	// (기본 Reflection 핸들러는 UObject를 반환해 모든 것을 받는다)
	virtual UClass* GetSupportedClass() const = 0;

	// 에셋 → data 객체. 다른 에셋 참조는 Resolver.ToLogicalPath로 주소 글자로 바꾼다.
	// 실패하면 Ctx에 에러를 남기고 nullptr
	virtual TSharedPtr<FDataBridgeObject> Export(const UObject* Asset, const FDataBridgeReferenceResolver& Resolver, FDataBridgeContext& Ctx) const = 0;

	// data 객체 → 에셋. 참조는 Resolver로만 해결한다 (직접 LoadObject 금지).
	// 생략된 필드는 기존 값 유지, 배열은 통째 교체. 실패하면 Ctx에 에러를 남기고 false
	virtual bool Import(const FDataBridgeObject& Data, UObject* Target, const FDataBridgeReferenceResolver& Resolver, FDataBridgeContext& Ctx) const = 0;


	// 클래스 → 설명서 (필드, 타입, 허용 값). Export/Import가 다루는 모양과 정확히 같아야 한다 (왕복 계약의 문서판).
	// 자기 클래스를 Schema에 등록하고(TryBeginClass → SetClass),
	// 필드가 가리키는 구조체와 Instanced 클래스도 함께 등록한다.
	virtual void Describe(const UClass* Class, FDataBridgeSchemaBuilder& Schema, FDataBridgeContext& Ctx) const = 0;

};
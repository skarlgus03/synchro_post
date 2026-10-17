#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"
#include "DataBridgeTypes.h"

class FDataBridgeContext;
class UClass;
class UObject;

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

	// 에셋 → data 객체. 실패하면 Ctx에 에러를 남기고 nullptr
	virtual TSharedPtr<FDataBridgeObject> Export(const UObject* Asset, FDataBridgeContext& Ctx) const = 0;

	// data 객체 → 에셋. 생략된 필드는 기존 값 유지, 배열은 통째 교체.
	// 실패하면 Ctx에 에러를 남기고 false (되돌리기는 파이프라인 책임)
	virtual bool Import(const FDataBridgeObject& Data, UObject* Asset, FDataBridgeContext& Ctx) const = 0;
};
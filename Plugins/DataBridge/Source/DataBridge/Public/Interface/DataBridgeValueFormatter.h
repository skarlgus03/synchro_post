#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"
#include "DataBridgeTypes.h"


class FDataBridgeContext;
class UScriptStruct;

// 특정 struct 타입 하나 ⇄ IR 값.
// 예: FGameplayTag ⇄ "Stat.Attack"
// Export 결과가 문자열이면 워커는 그 struct를 맵 키로도 쓸 수 있다
// (TMap<FGameplayTag, int32> → { "Stat.Attack": 100 })
// 기존에 있는 struct를, AI나 사람이 이해하기 쉬운 문자열로 표현하고 싶을 때 구현한다.
class IDataBridgeValueFormatter : public IModularFeature
{
public:
	static FName GetModularFeatureName()
	{
		static const FName Name(TEXT("DataBridgeValueFormatter"));
		return Name;
	}

	virtual ~IDataBridgeValueFormatter() = default;

	virtual UScriptStruct* GetSupportedStruct() const = 0;

	// StructData: GetSupportedStruct() 타입 인스턴스의 주소
	virtual TSharedPtr<FDataBridgeValue> Export(const void* StructData, FDataBridgeContext& Ctx) const = 0;

	virtual bool Import(const FDataBridgeValue& Value, void* StructData, FDataBridgeContext& Ctx) const = 0;
};
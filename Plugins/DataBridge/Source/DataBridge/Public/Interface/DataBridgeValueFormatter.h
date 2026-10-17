#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"

class FJsonValue;
class FDataBridgeContext;
class UScriptStruct;

// 특정 struct 타입 하나 ⇄ IR 값.
// 예: FGameplayTag ⇄ "Stat.Attack"
// Export 결과가 문자열이면 워커는 그 struct를 맵 키로도 쓸 수 있다
// (TMap<FGameplayTag, int32> → { "Stat.Attack": 100 })
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
	virtual TSharedPtr<FJsonValue> Export(const void* StructData, FDataBridgeContext& Ctx) const = 0;

	virtual bool Import(const FJsonValue& Value, void* StructData, FDataBridgeContext& Ctx) const = 0;
};
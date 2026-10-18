#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeatures.h"

class IDataBridgeFormat;
class IDataBridgeHandler;
class IDataBridgeValueFormatter;
class UClass;
class UScriptStruct;

// 등록 창구. 등록 목록은 엔진의 IModularFeatures를 통해 보관한다. 
// 여기는 등록 창구만 제공함.
class DATABRIDGE_API FDataBridgeRegistry
{
public:
	// 확장 모듈의 StartupModule / ShutdownModule에서 호출한다.
	template<typename TFeature>
	static void Register(TFeature& Feature)
	{
		IModularFeatures::Get().RegisterModularFeature(TFeature::GetModularFeatureName(), &Feature);
	}

	template <typename TFeature>
	static void Unregister(TFeature& Feature)
	{
		IModularFeatures::Get().UnregisterModularFeature(TFeature::GetModularFeatureName(), &Feature);
	}

	// "json", ".json", "JSON" 모두 허용
	static const IDataBridgeFormat* FindFormatByExtension(const FString& Extension);

	// 상속 거리가 가장 가까운 핸들러. 없으면 nullptr
	static const IDataBridgeHandler* FindHandler(const UClass* AssetClass);

	// struct가 정확히 일치하는 포매터. 없으면 nullptr
	static const IDataBridgeValueFormatter* FindValueFormatter(const UScriptStruct* Struct);
};

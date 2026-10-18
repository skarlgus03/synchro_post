#include "DataBridgeRegistry.h"

#include "DataBridgeTypes.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Interfaces/DataBridgeHandler.h"
#include "Interfaces/DataBridgeValueFormatter.h"
#include "UObject/Class.h"

namespace
{
	template <typename TFeature>
	TArray<TFeature*> GetAllFeatures()
	{
		// 조회는 엔진이 잠가주지 않으므로 직접 잠근다 (게임 스레드 외 호출 대비)
		IModularFeatures::FScopedLockModularFeatureList Lock;
		return IModularFeatures::Get().GetModularFeatureImplementations<TFeature>(TFeature::GetModularFeatureName());
	}

	// AssetClass가 SupportedClass로부터 몇 단계 아래 자식인지. 자식이 아니면 INDEX_NONE
	int32 GetInheritanceDistance(const UClass* AssetClass, const UClass* SupportedClass)
	{
		int32 Distance = 0;
		for (const UClass* It = AssetClass; It; It = It->GetSuperClass(), ++Distance)
		{
			if (It == SupportedClass)
			{
				return Distance;
			}
		}
		return INDEX_NONE;
	}
}

const IDataBridgeFormat* FDataBridgeRegistry::FindFormatByExtension(const FString& Extension)
{
	FString Wanted = Extension;
	Wanted.RemoveFromStart(TEXT("."));

	TArray<FString> Extensions;
	for (const IDataBridgeFormat* Format : GetAllFeatures<IDataBridgeFormat>())
	{
		Extensions.Reset();
		Format->GetFileExtensions(Extensions);

		const bool bMatches = Extensions.ContainsByPredicate([&Wanted](const FString& Ext)
			{
				return Ext.Equals(Wanted, ESearchCase::IgnoreCase);
			});

		if (bMatches)
		{
			return Format;
		}
	}
	return nullptr;
}

const IDataBridgeHandler* FDataBridgeRegistry::FindHandler(const UClass* AssetClass)
{
	if (!AssetClass)
	{
		return nullptr;
	}

	const IDataBridgeHandler* Best = nullptr;
	int32 BestDistance = MAX_int32;

	for (const IDataBridgeHandler* Handler : GetAllFeatures<IDataBridgeHandler>())
	{
		const int32 Distance = GetInheritanceDistance(AssetClass, Handler->GetSupportedClass());
		if (Distance == INDEX_NONE)
		{
			continue;
		}

		if (Distance < BestDistance)
		{
			Best = Handler;
			BestDistance = Distance;
		}
		else if (Distance == BestDistance)
		{
			UE_LOG(LogDataBridge, Warning,
				TEXT("Multiple handlers support '%s' at the same distance. Using the first registered one."),
				*GetNameSafe(Handler->GetSupportedClass()));
		}
	}
	return Best;
}

const IDataBridgeValueFormatter* FDataBridgeRegistry::FindValueFormatter(const UScriptStruct* Struct)
{
	if (!Struct)
	{
		return nullptr;
	}

	for (const IDataBridgeValueFormatter* Formatter : GetAllFeatures<IDataBridgeValueFormatter>())
	{
		if (Formatter->GetSupportedStruct() == Struct)
		{
			return Formatter;
		}
	}
	return nullptr;
}
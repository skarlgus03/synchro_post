#include "DataBridge.h"

#include "DataBridgeRegistry.h"
#include "DataBridgeTypes.h"
#include "Formats/DataBridgeJsonFormat.h"
#include "Formatters/DataBridgeGameplayTagFormatters.h"
#include "Handlers/DataBridgeReflectionHandler.h"

void FDataBridgeModule::StartupModule()
{
	JsonFormat = MakeUnique<FDataBridgeJsonFormat>();
	ReflectionHandler = MakeUnique<FDataBridgeReflectionHandler>();

	FDataBridgeRegistry::Register(*JsonFormat);
	FDataBridgeRegistry::Register(*ReflectionHandler);

	ValueFormatters.Add(MakeUnique<FDataBridgeGameplayTagFormatter>());
	ValueFormatters.Add(MakeUnique<FDataBridgeGameplayTagContainerFormatter>());
	for (const TUniquePtr<IDataBridgeValueFormatter>& Formatter : ValueFormatters)
	{
		FDataBridgeRegistry::Register(*Formatter);
	}

	// 임시 확인용: 창구 조회가 동작하는지 
	UE_LOG(LogDataBridge, Log, TEXT("DataBridge started. json format: %s, UObject handler: %s"),
		FDataBridgeRegistry::FindFormatByExtension(TEXT("json")) ? TEXT("OK") : TEXT("MISSING"),
		FDataBridgeRegistry::FindHandler(UObject::StaticClass()) ? TEXT("OK") : TEXT("MISSING"));
}

void FDataBridgeModule::ShutdownModule()
{
	for (const TUniquePtr<IDataBridgeValueFormatter>& Formatter : ValueFormatters)
	{
		FDataBridgeRegistry::Unregister(*Formatter);
	}
	ValueFormatters.Reset();

	if (ReflectionHandler)
	{
		FDataBridgeRegistry::Unregister(*ReflectionHandler);
		ReflectionHandler.Reset();
	}
	if (JsonFormat)
	{
		FDataBridgeRegistry::Unregister(*JsonFormat);
		JsonFormat.Reset();
	}
}

IMPLEMENT_MODULE(FDataBridgeModule, DataBridge)
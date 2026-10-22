#include "DataBridge.h"

#include "DataBridgeRegistry.h"
#include "DataBridgeTypes.h"
#include "Formats/DataBridgeJsonFormat.h"
#include "Formatters/DataBridgeGameplayTagFormatters.h"
#include "Formatters/DataBridgeTransformFormatter.h"
#include "Handlers/DataBridgeReflectionHandler.h"

void FDataBridgeModule::StartupModule()
{
	JsonFormat = MakeUnique<FDataBridgeJsonFormat>();
	ReflectionHandler = MakeUnique<FDataBridgeReflectionHandler>();

	FDataBridgeRegistry::Register(*JsonFormat);
	FDataBridgeRegistry::Register(*ReflectionHandler);

	ValueFormatters.Add(MakeUnique<FDataBridgeGameplayTagFormatter>());
	ValueFormatters.Add(MakeUnique<FDataBridgeGameplayTagContainerFormatter>());
	ValueFormatters.Add(MakeUnique<FDataBridgeTransformFormatter>());

	for (const TUniquePtr<IDataBridgeValueFormatter>& Formatter : ValueFormatters)
	{
		FDataBridgeRegistry::Register(*Formatter);
	}
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
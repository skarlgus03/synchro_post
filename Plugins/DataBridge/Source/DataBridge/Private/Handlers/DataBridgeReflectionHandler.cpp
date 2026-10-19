#include "Handlers/DataBridgeReflectionHandler.h"
#include "Handlers/DataBridgeExportWalker.h"
#include "Dom/JsonObject.h"
#include "UObject/Object.h"

UClass* FDataBridgeReflectionHandler::GetSupportedClass() const
{
	return UObject::StaticClass();
}

TSharedPtr<FDataBridgeObject> FDataBridgeReflectionHandler::Export(const UObject* Asset, const FDataBridgeReferenceResolver& Resolver, FDataBridgeContext& Ctx) const
{
	FDataBridgeExportWalker Walker(Resolver, Ctx);
	return Walker.ExportObject(Asset);
}

bool FDataBridgeReflectionHandler::Import(const FDataBridgeObject& Data, UObject* Target, const FDataBridgeReferenceResolver& Resolver, FDataBridgeContext& Ctx) const
{
	Ctx.AddError(TEXT("Reflection handler: Import not implemented yet"));
	return false;
}
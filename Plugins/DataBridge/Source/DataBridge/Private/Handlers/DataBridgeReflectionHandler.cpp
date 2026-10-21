#include "Handlers/DataBridgeReflectionHandler.h"
#include "Handlers/DataBridgeExportWalker.h"
#include "Handlers/DataBridgeImportWalker.h"
#include "Dom/JsonObject.h"
#include "UObject/Object.h"
#include "DataBridgeSchemaBuilder.h"
#include "Handlers/DataBridgeDescribeWalker.h"

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
	FDataBridgeImportWalker Walker(Resolver, Ctx);
	return Walker.ImportObject(Data, Target);
}

void FDataBridgeReflectionHandler::Describe(const UClass* Class, FDataBridgeSchemaBuilder& Schema, FDataBridgeContext& Ctx) const
{
	FDataBridgeDescribeWalker Walker(Schema, Ctx);
	Walker.DescribeClass(Class);
}
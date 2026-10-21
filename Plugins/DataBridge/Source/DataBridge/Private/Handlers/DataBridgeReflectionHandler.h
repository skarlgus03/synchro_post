#pragma once

#include "Interfaces/DataBridgeHandler.h"

// 기본 핸들러: UPROPERTY 리플렉션으로 모든 UObject를 처리한다.
class FDataBridgeReflectionHandler : public IDataBridgeHandler
{
public:
	virtual UClass* GetSupportedClass() const override;
	virtual TSharedPtr<FDataBridgeObject> Export(const UObject* Asset, const FDataBridgeReferenceResolver& Resolver, FDataBridgeContext& Ctx) const override;
	virtual bool Import(const FDataBridgeObject& Data, UObject* Target, const FDataBridgeReferenceResolver& Resolver, FDataBridgeContext& Ctx) const override;
	virtual void Describe(const UClass* Class, FDataBridgeSchemaBuilder& Schema, FDataBridgeContext& Ctx) const override;
};
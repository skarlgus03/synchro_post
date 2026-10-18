#pragma once

#include "Interfaces/DataBridgeHandler.h"

// 기본 핸들러: UPROPERTY 리플렉션으로 모든 UObject를 처리한다.
class FDataBridgeReflectionHandler : public IDataBridgeHandler
{
public:
	virtual UClass* GetSupportedClass() const override;
	virtual TSharedPtr<FDataBridgeObject> Export(const UObject* Asset, FDataBridgeContext& Ctx) const override;
	virtual bool Import(const FDataBridgeObject& Data, UObject* Asset, FDataBridgeContext& Ctx) const override;
};
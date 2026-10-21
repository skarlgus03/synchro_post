#pragma once

#include "Interfaces/DataBridgeValueFormatter.h"

// FGameplayTag ⇄ "Stat.Attack"   (빈 태그 ⇄ "")
class FDataBridgeGameplayTagFormatter : public IDataBridgeValueFormatter
{
public:
	virtual UScriptStruct* GetSupportedStruct() const override;
	virtual TSharedPtr<FDataBridgeValue> Export(const void* StructData, FDataBridgeContext& Ctx) const override;
	virtual bool Import(const FDataBridgeValue& Value, void* StructData, FDataBridgeContext& Ctx) const override;
	virtual TSharedRef<FDataBridgeObject> DescribeValue() const override;
};

// FGameplayTagContainer ⇄ ["Trait.Fire", "Trait.Undead"]
class FDataBridgeGameplayTagContainerFormatter : public IDataBridgeValueFormatter
{
public:
	virtual UScriptStruct* GetSupportedStruct() const override;
	virtual TSharedPtr<FDataBridgeValue> Export(const void* StructData, FDataBridgeContext& Ctx) const override;
	virtual bool Import(const FDataBridgeValue& Value, void* StructData, FDataBridgeContext& Ctx) const override;
	virtual TSharedRef<FDataBridgeObject> DescribeValue() const override;
};
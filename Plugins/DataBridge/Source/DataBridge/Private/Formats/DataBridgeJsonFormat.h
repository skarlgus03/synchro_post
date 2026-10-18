#pragma once

#include "CoreMinimal.h"
#include "Interfaces/DataBridgeFormat.h"


class DATABRIDGE_API FDataBridgeJsonFormat : public IDataBridgeFormat
{
public:
	virtual FName GetFormatName() const override;
	virtual void GetFileExtensions(TArray<FString>& OutExtensions) const override;
	virtual bool Read(const FString& Text, TSharedPtr<FDataBridgeObject>& OutRoot, FDataBridgeContext& Ctx) const override;
	virtual bool Write(const TSharedRef<FDataBridgeObject>& Root, FString& OutText, FDataBridgeContext& Ctx) const override;
};

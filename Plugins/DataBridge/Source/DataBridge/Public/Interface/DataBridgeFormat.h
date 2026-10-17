#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"

class FJsonObject;
class FDataBridgeContext;

// 텍스트 ⇄ IR(봉투 전체). 에셋에 대해서는 아무것도 모른다.
class IDataBridgeFormat : public IModularFeature
{
public:
	static FName GetModularFeatureName()
	{
		static const FName Name(TEXT("DataBridgeFormat"));
		return Name;
	}

	virtual ~IDataBridgeFormat() = default;

	// 포맷 식별자 (예: "json")
	virtual FName GetFormatName() const = 0;

	// 처리하는 파일 확장자, 점 없이 (예: "json")
	virtual void GetFileExtensions(TArray<FString>& OutExtensions) const = 0;

	// 텍스트 → IR. 실패하면 Ctx에 에러를 남기고 false
	virtual bool Read(const FString& Text, TSharedPtr<FJsonObject>& OutRoot, FDataBridgeContext& Ctx) const = 0;

	// IR → 텍스트. 이 포맷으로 표현할 수 없는 모양이면 Ctx에 에러를 남기고 false
	virtual bool Write(const TSharedRef<FJsonObject>& Root, FString& OutText, FDataBridgeContext& Ctx) const = 0;
};
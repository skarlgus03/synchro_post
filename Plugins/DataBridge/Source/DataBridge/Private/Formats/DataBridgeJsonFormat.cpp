#include "Formats/DataBridgeJsonFormat.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FName FDataBridgeJsonFormat::GetFormatName() const
{
	static const FName Name(TEXT("json"));
	return Name;
}

void FDataBridgeJsonFormat::GetFileExtensions(TArray<FString>& OutExtensions) const
{
	OutExtensions.Add(TEXT("json"));
}

bool FDataBridgeJsonFormat::Read(const FString& Text, TSharedPtr<FDataBridgeObject>& OutRoot, FDataBridgeContext& Ctx) const
{
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);

	if (!FJsonSerializer::Deserialize(Reader, OutRoot) || !OutRoot.IsValid())
	{
		Ctx.AddError(FString::Printf(TEXT("JSON parse failed (line %u, column %u): %s"),
			Reader->GetLineNumber(), Reader->GetCharacterNumber(), *Reader->GetErrorMessage()));
		OutRoot.Reset();
		return false;
	}
	return true;
}

bool FDataBridgeJsonFormat::Write(const TSharedRef<FDataBridgeObject>& Root, FString& OutText, FDataBridgeContext& Ctx) const
{
	OutText.Reset();

	// 들여쓰기 출력: 사람/AI가 읽기 쉽고 Git diff가 줄 단위로 보인다
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutText);

	if (!FJsonSerializer::Serialize(Root, Writer))
	{
		Ctx.AddError(TEXT("JSON serialization failed"));
		return false;
	}
	return true;
}
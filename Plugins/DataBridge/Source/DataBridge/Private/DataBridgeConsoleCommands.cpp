#include "DataBridgePipeline.h"
#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace DataBridgeConsole
{
	// "/Script/Module.Class'/Game/A/DA_X.DA_X'" (Copy Reference 형식)도 받는다
	FString StripReferenceQuotes(const FString& Arg)
	{
		int32 QuoteIndex = INDEX_NONE;
		if (Arg.FindChar(TEXT('\''), QuoteIndex))
		{
			FString Inner = Arg.Mid(QuoteIndex + 1);
			Inner.RemoveFromEnd(TEXT("'"));
			return Inner;
		}
		return Arg;
	}

	void Export(const TArray<FString>& Args)
	{
		if (Args.Num() == 0)
		{
			UE_LOG(LogDataBridge, Warning, TEXT("Usage: DataBridge.Export /Game/Path/Asset [/Game/Path/Other ...]"));
			return;
		}

		FDataBridgeContext Ctx;
		TArray<UObject*> Assets;
		for (const FString& Arg : Args)
		{
			const FString ObjectPath = FDataBridgeReferenceResolver::NormalizeObjectPath(StripReferenceQuotes(Arg));
			if (UObject* Asset = LoadObject<UObject>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				Assets.Add(Asset);
			}
			else
			{
				Ctx.AddError(FString::Printf(TEXT("Asset '%s' not found"), *Arg));
			}
		}

		TSharedPtr<FDataBridgeObject> Root;
		if (!Ctx.HasErrors())
		{
			Root = FDataBridgePipeline::Export(Assets, Ctx);
		}

		if (Ctx.GetIssues().Num() > 0)
		{
			UE_LOG(LogDataBridge, Warning, TEXT("DataBridge.Export issues:\n%s"), *Ctx.BuildReport());
		}
		if (!Root.IsValid())
		{
			UE_LOG(LogDataBridge, Error, TEXT("DataBridge.Export failed"));
			return;
		}

		const IDataBridgeFormat* Json = FDataBridgeRegistry::FindFormatByExtension(TEXT("json"));
		FString Text;
		if (!Json || !Json->Write(Root.ToSharedRef(), Text, Ctx))
		{
			UE_LOG(LogDataBridge, Error, TEXT("DataBridge.Export: JSON write failed\n%s"), *Ctx.BuildReport());
			return;
		}

		// 로그 + 파일 (한글 확인은 파일로 보는 게 편하다)
		const FString FilePath = FPaths::ProjectSavedDir() / TEXT("DataBridge") / TEXT("Export.json");
		FFileHelper::SaveStringToFile(Text, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, /*bCullEmpty*/ false);
		for (const FString& Line : Lines)
		{
			UE_LOG(LogDataBridge, Log, TEXT("%s"), *Line);
		}
		UE_LOG(LogDataBridge, Log, TEXT("Saved to %s"), *FPaths::ConvertRelativePathToFull(FilePath));
	}
}

static FAutoConsoleCommand GDataBridgeExportCommand(
	TEXT("DataBridge.Export"),
	TEXT("Exports assets as DataBridge JSON to the log and Saved/DataBridge/Export.json. Usage: DataBridge.Export /Game/Path/Asset [...]"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&DataBridgeConsole::Export));
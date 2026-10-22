#include "DataBridgePipeline.h"
#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "DataBridgeImportPlan.h"
#include "DataBridgeSchemaBuilder.h"

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

	// 파일 경로(상대 경로면 프로젝트 폴더 기준) → IR 봉투
	TSharedPtr<FDataBridgeObject> LoadDocument(const FString& Arg, FDataBridgeContext& Ctx)
	{
		FString FilePath = Arg.TrimQuotes();
		if (FPaths::IsRelative(FilePath))
		{
			FilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FilePath);
		}

		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *FilePath))
		{
			Ctx.AddError(FString::Printf(TEXT("Cannot read file '%s'"), *FilePath));
			return nullptr;
		}

		const IDataBridgeFormat* Format = FDataBridgeRegistry::FindFormatByExtension(FPaths::GetExtension(FilePath));
		if (!Format)
		{
			Ctx.AddError(FString::Printf(TEXT("No format registered for '.%s'"), *FPaths::GetExtension(FilePath)));
			return nullptr;
		}

		TSharedPtr<FDataBridgeObject> Root;
		Format->Read(Text, Root, Ctx);
		return Root;
	}

	void LogLines(const FString& Text, ELogVerbosity::Type Verbosity)
	{
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines);
		for (const FString& Line : Lines)
		{
			if (Verbosity == ELogVerbosity::Error)
			{
				UE_LOG(LogDataBridge, Error, TEXT("%s"), *Line);
			}
			else
			{
				UE_LOG(LogDataBridge, Log, TEXT("%s"), *Line);
			}
		}
	}

	// DryRun과 Import가 같이 쓰는 출력: 계획 → 변경 목록 → 이슈
	void LogResult(bool bPlanned, const FDataBridgeImportPlan& Plan, bool bRehearsed, const TArray<FString>& Changes, const FDataBridgeContext& Ctx)
	{
		if (bPlanned)
		{
			UE_LOG(LogDataBridge, Log, TEXT("---- Plan ----"));
			LogLines(Plan.Describe(), ELogVerbosity::Log);
		}
		if (bRehearsed)
		{
			UE_LOG(LogDataBridge, Log, TEXT("---- Changes (%d) ----"), Changes.Num());
			for (const FString& Change : Changes)
			{
				UE_LOG(LogDataBridge, Log, TEXT("%s"), *Change);
			}
		}
		if (Ctx.GetIssues().Num() > 0)
		{
			UE_LOG(LogDataBridge, Log, TEXT("---- Issues ----"));
			LogLines(Ctx.BuildReport(), Ctx.HasErrors() ? ELogVerbosity::Error : ELogVerbosity::Log);
		}
	}

	void DryRun(const TArray<FString>& Args)
	{
		if (Args.Num() != 1)
		{
			UE_LOG(LogDataBridge, Warning, TEXT("Usage: DataBridge.DryRun <file.json>   (relative paths start at the project folder)"));
			return;
		}

		FDataBridgeContext Ctx;
		FDataBridgeImportPlan Plan;
		TArray<FString> Changes;

		const TSharedPtr<FDataBridgeObject> Root = LoadDocument(Args[0], Ctx);
		const bool bPlanned = Root.IsValid() && FDataBridgePipeline::BuildImportPlan(*Root, Plan, Ctx);
		const bool bRehearsed = bPlanned && FDataBridgePipeline::Rehearse(Plan, Ctx, &Changes);

		LogResult(bPlanned, Plan, bRehearsed, Changes, Ctx);

		if (bRehearsed)
		{
			UE_LOG(LogDataBridge, Log, TEXT("Dry run OK. Nothing was changed."));
		}
		else
		{
			UE_LOG(LogDataBridge, Error, TEXT("Dry run FAILED. Nothing was changed."));
		}
	}

	void Import(const TArray<FString>& Args)
	{
		if (Args.Num() != 1)
		{
			UE_LOG(LogDataBridge, Warning, TEXT("Usage: DataBridge.Import <file.json>   (relative paths start at the project folder)"));
			return;
		}

		FDataBridgeContext Ctx;
		FDataBridgeImportPlan Plan;
		TArray<FString> Changes;

		// 단계를 직접 나눠 부른다: 어디서 멈췄는지에 따라 사용자에게 할 말이 다르다
		const TSharedPtr<FDataBridgeObject> Root = LoadDocument(Args[0], Ctx);
		const bool bPlanned = Root.IsValid() && FDataBridgePipeline::BuildImportPlan(*Root, Plan, Ctx);
		const bool bRehearsed = bPlanned && FDataBridgePipeline::Rehearse(Plan, Ctx, &Changes);
		const bool bCommitted = bRehearsed && FDataBridgePipeline::Commit(Plan, Ctx);

		LogResult(bPlanned, Plan, bRehearsed, Changes, Ctx);

		if (bCommitted)
		{
			UE_LOG(LogDataBridge, Log, TEXT("Import OK. %d updated (Ctrl+Z to undo), %d created (not undoable). Nothing was saved."),
				Plan.Num(EDataBridgeImportAction::Update), Plan.Num(EDataBridgeImportAction::Create));
		}
		else if (bRehearsed)
		{
			UE_LOG(LogDataBridge, Error, TEXT("Import FAILED while applying. Rolled back."));
		}
		else
		{
			UE_LOG(LogDataBridge, Error, TEXT("Import FAILED before applying. Nothing was changed."));
		}
	}

	// IR 문서 → Saved/DataBridge/<FileName>. 저장된 전체 경로를 돌려준다 (실패하면 빈 문자열)
	FString SaveDocument(const TSharedRef<FDataBridgeObject>& Root, const FString& FileName, FDataBridgeContext& Ctx)
	{
		const IDataBridgeFormat* Json = FDataBridgeRegistry::FindFormatByExtension(TEXT("json"));
		FString Text;
		if (!Json || !Json->Write(Root, Text, Ctx))
		{
			Ctx.AddError(TEXT("JSON write failed"));
			return FString();
		}

		const FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("DataBridge") / FileName);
		if (!FFileHelper::SaveStringToFile(Text, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			Ctx.AddError(FString::Printf(TEXT("Cannot write file '%s'"), *FilePath));
			return FString();
		}
		return FilePath;
	}

	void Describe(const TArray<FString>& Args)
	{
		using namespace DataBridge;

		if (Args.Num() < 1)
		{
			UE_LOG(LogDataBridge, Warning, TEXT("Usage: DataBridge.Describe <Class> [ContentRoot ...]   e.g. DataBridge.Describe SkillDataAsset /Game/SynchroPost /Game/Niagara"));
			return;
		}

		FDataBridgeContext Ctx;

		// 첫 인자 = 클래스 이름, 나머지 인자 = 카탈로그 범위
		const TArray<FString> Roots(Args.GetData() + 1, Args.Num() - 1);
		const UClass* Class = FDataBridgeReferenceResolver::FindClass(StripReferenceQuotes(Args[0]), Ctx);
		const TSharedPtr<FDataBridgeObject> Root = Class 
			? FDataBridgePipeline::Describe(Class, Ctx, Roots) : nullptr;
		const FString FilePath = Root.IsValid()
			? SaveDocument(Root.ToSharedRef(), FString::Printf(TEXT("Describe_%s.json"), *Class->GetName()), Ctx)
			: FString();

		if (Ctx.GetIssues().Num() > 0)
		{
			UE_LOG(LogDataBridge, Log, TEXT("---- Issues ----"));
			LogLines(Ctx.BuildReport(), Ctx.HasErrors() ? ELogVerbosity::Error : ELogVerbosity::Log);
		}
		if (FilePath.IsEmpty())
		{
			UE_LOG(LogDataBridge, Error, TEXT("Describe FAILED."));
			return;
		}

		// 설명서는 길어서 로그에는 요약만. 주석 없는 필드는 바로 채울 수 있게 목록으로
		const auto CountEntries = [&Root](const TCHAR* Field)
			{
				const TSharedPtr<FDataBridgeObject>* Table = nullptr;
				return Root->TryGetObjectField(Field, Table) ? (*Table)->Values.Num() : 0;
			};
		const TArray<TSharedPtr<FDataBridgeValue>>* Undocumented = nullptr;
		Root->TryGetArrayField(SchemaKeys::Undocumented, Undocumented);

		UE_LOG(LogDataBridge, Log, TEXT("Describe OK. %d classes, %d structs, %d undocumented fields."),
			CountEntries(SchemaKeys::Classes), CountEntries(SchemaKeys::Structs), Undocumented ? Undocumented->Num() : 0);
		if (Undocumented)
		{
			for (const TSharedPtr<FDataBridgeValue>& Where : *Undocumented)
			{
				UE_LOG(LogDataBridge, Log, TEXT("  undocumented: %s"), *Where->AsString());
			}
		}
		UE_LOG(LogDataBridge, Log, TEXT("Saved to %s"), *FilePath);
	}
}

static FAutoConsoleCommand GDataBridgeExportCommand(
	TEXT("DataBridge.Export"),
	TEXT("Exports assets as DataBridge JSON to the log and Saved/DataBridge/Export.json. Usage: DataBridge.Export /Game/Path/Asset [...]"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&DataBridgeConsole::Export));

static FAutoConsoleCommand GDataBridgeDryRunCommand(
	TEXT("DataBridge.DryRun"),
	TEXT("Plans and rehearses an import on temporary copies, then lists the changes. Never modifies assets. Usage: DataBridge.DryRun <file.json>"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&DataBridgeConsole::DryRun));

static FAutoConsoleCommand GDataBridgeImportCommand(
	TEXT("DataBridge.Import"),
	TEXT("Plans, rehearses, then applies an import as one undoable transaction. Does not save. Usage: DataBridge.Import <file.json>"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&DataBridgeConsole::Import));

static FAutoConsoleCommand GDataBridgeDescribeCommand(
	TEXT("DataBridge.Describe"),
	TEXT("Writes a description of a class (fields, types, allowed values, defaults) for AI authoring. Usage: DataBridge.Describe <Class> [ContextRoot ...]"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&DataBridgeConsole::Describe));
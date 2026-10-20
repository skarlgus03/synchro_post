#include "DataBridgePipeline.h"

#include "DataBridgeDiff.h"
#include "DataBridgeImportPlan.h"
#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Interfaces/DataBridgeHandler.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/DataValidation.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace DataBridgePipelineApply
{
	// 이번 적용 한 번의 준비물: 주소록 + 대상 오브젝트(Plan.Entries와 같은 순서)
	struct FApplyTargets
	{
		FDataBridgeReferenceResolver Resolver;
		TArray<TStrongObjectPtr<UObject>> Objects;   // 연습이면 복제본, 실전이면 진짜. GC로부터 붙잡아 둔다
	};

	// IsDataValid 결과를 우리 일지로 옮긴다
	void Validate(UObject* Target, FDataBridgeContext& Ctx)
	{
		FDataValidationContext ValidationContext;
		const EDataValidationResult Result = Target->IsDataValid(ValidationContext);

		for (const FDataValidationContext::FIssue& Issue : ValidationContext.GetIssues())
		{
			const FText Message = Issue.TokenizedMessage.IsValid() ? Issue.TokenizedMessage->ToText() : Issue.Message;
			if (Issue.Severity == EMessageSeverity::Error)
			{
				Ctx.AddError(TEXT("IsDataValid: ") + Message.ToString());
			}
			else if (Issue.Severity == EMessageSeverity::Warning || Issue.Severity == EMessageSeverity::PerformanceWarning)
			{
				Ctx.AddWarning(TEXT("IsDataValid: ") + Message.ToString());
			}
		}

		if (Result == EDataValidationResult::Invalid && ValidationContext.GetNumErrors() == 0)
		{
			Ctx.AddError(TEXT("IsDataValid reported the asset as invalid"));
		}
	}

	// 연습과 실전이 공통으로 타는 과정. 대상만 다르고 코드는 같다.
	// 전부 Import → 전부 PostEditChange → 전부 IsDataValid
	bool ApplyPlan(const FDataBridgeImportPlan& Plan, FApplyTargets& Targets, FDataBridgeContext& Ctx)
	{
		using namespace DataBridge;
		check(Targets.Objects.Num() == Plan.Entries.Num());

		const int32 ErrorsBefore = Ctx.GetErrorCount();

		// 1. 전부 Import (주소록이 다 채워진 뒤라서, 같은 문서의 다른 에셋을 참조해도 찾을 수 있다)
		for (int32 Index = 0; Index < Plan.Entries.Num(); ++Index)
		{
			const FDataBridgeImportEntry& Entry = Plan.Entries[Index];
			UObject* Target = Targets.Objects[Index].Get();

			// 새 에셋은 Undo 대상이 아니다 (에디터의 에셋 생성과 같은 규칙).
			// 끄지 않으면 새 에셋 안에 만든 Instanced 자식만 Undo 때 Garbage가 되어 에셋이 망가진다.
			// 연습 중에는 원래 트랜잭션이 없으므로(GUndo == nullptr) 아무 영향 없음
			TGuardValue<ITransaction*> SuppressUndo(GUndo, Entry.Action == EDataBridgeImportAction::Create ? nullptr : GUndo);

			FDataBridgePathScope AssetsScope(Ctx, Keys::Assets);
			FDataBridgePathScope IndexScope(Ctx, Entry.SourceIndex);
			FDataBridgePathScope DataScope(Ctx, Keys::Data);

			const IDataBridgeHandler* Handler = FDataBridgeRegistry::FindHandler(Target->GetClass());
			if (!Handler)
			{
				Ctx.AddError(FString::Printf(TEXT("No handler for class '%s'"), *Target->GetClass()->GetPathName()));
				continue;
			}
			if (Entry.Data.IsValid())
			{
				Handler->Import(*Entry.Data, Target, Targets.Resolver, Ctx);
			}
		}

		// 2. 전부 넣은 뒤에 마무리 호출 (A가 B를 참조할 때 B가 이미 채워져 있도록)
		for (int32 Index = 0; Index < Plan.Entries.Num(); ++Index)
		{
			// PostEditChange 안에서 클래스가 스스로 Modify를 부를 수도 있으니 같은 규칙 적용
			TGuardValue<ITransaction*> SuppressUndo(GUndo, Plan.Entries[Index].Action == EDataBridgeImportAction::Create ? nullptr : GUndo);
			Targets.Objects[Index]->PostEditChange();
		}

		// 3. 게임 규칙 검사
		for (int32 Index = 0; Index < Plan.Entries.Num(); ++Index)
		{
			FDataBridgePathScope AssetsScope(Ctx, Keys::Assets);
			FDataBridgePathScope IndexScope(Ctx, Plan.Entries[Index].SourceIndex);
			Validate(Targets.Objects[Index].Get(), Ctx);
		}

		return Ctx.GetErrorCount() == ErrorsBefore;
	}

	// 에셋 하나를 IR로 (diff용). 실패해도 연습 결과에는 영향 없음
	TSharedPtr<FDataBridgeValue> ExportForDiff(const UObject* Object, const FDataBridgeReferenceResolver& Resolver, FDataBridgeContext& Ctx)
	{
		const IDataBridgeHandler* Handler = Object ? FDataBridgeRegistry::FindHandler(Object->GetClass()) : nullptr;
		const TSharedPtr<FDataBridgeObject> Data = Handler ? Handler->Export(Object, Resolver, Ctx) : nullptr;
		return Data.IsValid() ? MakeShared<FJsonValueObject>(Data) : TSharedPtr<FDataBridgeValue>();
	}
}

// 연습
bool FDataBridgePipeline::Rehearse(const FDataBridgeImportPlan& Plan, FDataBridgeContext& Ctx, TArray<FString>* OutChanges)
{
	using namespace DataBridgePipelineApply;
	using namespace DataBridge;

	const int32 ErrorsBefore = Ctx.GetErrorCount();
	UPackage* Transient = GetTransientPackage();

	FApplyTargets Targets;
	TArray<TSharedPtr<FDataBridgeValue>> BeforeData;   // diff용: 적용 전 진짜 에셋의 IR (Create는 비어 있음)
	const FDataBridgeReferenceResolver EmptyResolver;

	// 1. 연습용 복제본 준비 (진짜는 읽기만)
	for (const FDataBridgeImportEntry& Entry : Plan.Entries)
	{
		FDataBridgePathScope AssetsScope(Ctx, Keys::Assets);
		FDataBridgePathScope IndexScope(Ctx, Entry.SourceIndex);

		UClass* Class = Entry.Class.Get();
		if (!Class)
		{
			FDataBridgePathScope ClassScope(Ctx, Keys::Class);
			Ctx.AddError(TEXT("Class is no longer available (was it a Blueprint that got unloaded?)"));
			continue;
		}

		const FName CopyName = MakeUniqueObjectName(Transient, Class, FName(*(Entry.AssetName + TEXT("_Rehearsal"))));
		UObject* Copy = nullptr;

		if (Entry.Action == EDataBridgeImportAction::Update)
		{
			UObject* Real = LoadObject<UObject>(nullptr, *Entry.GetObjectPath(), nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (!Real)
			{
				FDataBridgePathScope PathScope(Ctx, Keys::Path);
				Ctx.AddError(TEXT("Existing asset could not be loaded"));
				continue;
			}
			if (OutChanges)
			{
				BeforeData.Add(ExportForDiff(Real, EmptyResolver, Ctx));
			}
			// 기존 상태 그대로 복제 (Instanced 서브오브젝트까지)
			Copy = DuplicateObject<UObject>(Real, Transient, CopyName);
		}
		else
		{
			if (OutChanges)
			{
				BeforeData.Add(nullptr);
			}
			Copy = NewObject<UObject>(Transient, Class, CopyName);
		}

		// 복제본은 에셋이 아니다: GC가 수거할 수 있게, Undo 기록에 섞이지 않게
		Copy->ClearFlags(RF_Public | RF_Standalone | RF_Transactional);

		Targets.Objects.Emplace(Copy);
		Targets.Resolver.Register(Entry.PackageName, Copy);
	}

	// 준비 단계에서 실패하면 적용하지 않는다
	if (Ctx.GetErrorCount() > ErrorsBefore || Targets.Objects.Num() != Plan.Entries.Num())
	{
		for (const TStrongObjectPtr<UObject>& Copy : Targets.Objects)
		{
			Copy->MarkAsGarbage();
		}
		return false;
	}

	// 2. 공통 과정 (실전과 같은 코드)
	ApplyPlan(Plan, Targets, Ctx);

	// 3. 변경 목록: 복제본을 원래 논리 경로로 Export해서 적용 전과 비교
	if (OutChanges)
	{
		for (int32 Index = 0; Index < Plan.Entries.Num(); ++Index)
		{
			const FDataBridgeImportEntry& Entry = Plan.Entries[Index];
			const FString Prefix = FString::Printf(TEXT("assets[%d]"), Entry.SourceIndex);

			if (Entry.Action == EDataBridgeImportAction::Create)
			{
				OutChanges->Add(FString::Printf(TEXT("%s %s: new asset"), *Prefix, *Entry.PackageName));
				continue;
			}

			const TSharedPtr<FDataBridgeValue> After = ExportForDiff(Targets.Objects[Index].Get(), Targets.Resolver, Ctx);
			DataBridgeDiff::Collect(BeforeData[Index], After, Prefix + TEXT(".data"), *OutChanges);
		}
	}

	// 4. 복제본 버리기
	for (const TStrongObjectPtr<UObject>& Copy : Targets.Objects)
	{
		Copy->MarkAsGarbage();
	}

	return Ctx.GetErrorCount() == ErrorsBefore;
}
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
#include "AssetRegistry/AssetRegistryModule.h"
#include "DataBridgeTransaction.h"

namespace DataBridgePipelineApply
{
	// 이번 적용 한 번의 준비물: 주소록 + 대상 오브젝트(Plan.Entries와 같은 순서)
	struct FApplyTargets
	{
		FDataBridgeReferenceResolver Resolver;
		TArray<TStrongObjectPtr<UObject>> Objects;   // 연습이면 복제본, 실전이면 진짜. GC로부터 붙잡아 둔다
	};

	// IsDataValid 결과를 우리 일지로 옮긴다
	void Validate(const UObject* Target, FDataBridgeContext& Ctx)
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

	// 실패한 Commit이 만든 새 에셋을 없던 일로. AssetRegistry에는 아직 알리지 않았으므로 알릴 것도 없다
	void DiscardCreatedAsset(UObject* Asset)
	{
		Asset->ClearFlags(RF_Public | RF_Standalone);
		// 같은 경로로 다시 Import할 수 있게 자리를 비운다.
		Asset->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
		Asset->MarkAsGarbage();
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

#if WITH_DEV_AUTOMATION_TESTS
bool FDataBridgePipeline::bFailNextCommitForTest = false;
#endif

// 실전
bool FDataBridgePipeline::Commit(const FDataBridgeImportPlan& Plan, FDataBridgeContext& Ctx)
{
	using namespace DataBridgePipelineApply;
	using namespace DataBridge;

#if WITH_DEV_AUTOMATION_TESTS
	const bool bInjectFailure = bFailNextCommitForTest;
	bFailNextCommitForTest = false;
#endif

	const int32 ErrorsBefore = Ctx.GetErrorCount();

	if (!FDataBridgeTransaction::CanBegin(Ctx))
	{
		return false;
	}

	FApplyTargets Targets;
	TArray<UObject*> Created;   // Targets.Objects가 붙잡고 있으므로 여기는 그냥 포인터

	// 1. 진짜 대상 확보. 트랜잭션 밖에서 한다 → 새 에셋 생성이 Undo 기록에 남지 않는다
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

		UObject* Target = nullptr;

		if (Entry.Action == EDataBridgeImportAction::Update)
		{
			FDataBridgePathScope PathScope(Ctx, Keys::Path);

			Target = LoadObject<UObject>(nullptr, *Entry.GetObjectPath(), nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (!Target || !Target->IsA(Class))
			{
				Ctx.AddError(TEXT("Existing asset could not be loaded, or its class changed after planning"));
				continue;
			}
			// Modify는 기록에 실패해도 패키지를 dirty로 만든다. 건드리기 전에 확인
			if (!Target->HasAnyFlags(RF_Transactional))
			{
				Ctx.AddError(TEXT("Asset is not transactional; its changes could not be undone"));
				continue;
			}
		}
		else
		{
			UPackage* Package = CreatePackage(*Entry.PackageName);
			if (FindObject<UObject>(Package, *Entry.AssetName))
			{
				FDataBridgePathScope PathScope(Ctx, Keys::Path);
				Ctx.AddError(TEXT("An asset appeared at this path after planning"));
				continue;
			}

			Target = NewObject<UObject>(Package, Class, FName(*Entry.AssetName), RF_Public | RF_Standalone | RF_Transactional);
			Created.Add(Target);
		}

		Targets.Objects.Emplace(Target);
		Targets.Resolver.Register(Entry.PackageName, Target);
	}

	if (Ctx.GetErrorCount() > ErrorsBefore || Targets.Objects.Num() != Plan.Entries.Num())
	{
		for (UObject* Asset : Created)
		{
			DiscardCreatedAsset(Asset);
		}
		return false;
	}

	// 2~3. 하나의 Undo 기록 안에서 적용
	bool bApplied = false;
	{
		FDataBridgeTransaction Transaction(NSLOCTEXT("DataBridge", "ImportTransaction", "DataBridge Import"));

		// 2. 기존 에셋의 현재 상태 기록.
		//    Import는 기존 서브오브젝트를 고치지 않으므로(Instanced는 항상 새로 만든다) 본체만 기록하면 된다
		bool bHasUpdates = false;
		for (int32 Index = 0; Index < Plan.Entries.Num(); ++Index)
		{
			if (Plan.Entries[Index].Action == EDataBridgeImportAction::Update)
			{
				Targets.Objects[Index]->Modify();
				bHasUpdates = true;
			}
		}

		// 3. 공통 과정 (연습과 같은 코드)
		bApplied = ApplyPlan(Plan, Targets, Ctx);

#if WITH_DEV_AUTOMATION_TESTS
		if (bApplied && bInjectFailure)
		{
			Ctx.AddError(TEXT("Injected commit failure (test)"));
			bApplied = false;
		}
#endif

		if (!bApplied)
		{
			switch (Transaction.UndoIfOwned())
			{
			case EDataBridgeUndoResult::Undone:
				break;
			case EDataBridgeUndoResult::NotOwned:
				// Create만 있으면 기록이 비어서 엔진이 버린 것 → 정상.
				// Update가 있었는데 우리 기록이 없다면 이상 상황
				if (bHasUpdates)
				{
					Ctx.AddError(TEXT("Rollback could not find the import transaction; existing assets may be partially modified"));
				}
				break;
			case EDataBridgeUndoResult::Failed:
				Ctx.AddError(TEXT("Rollback was refused by the editor; existing assets may be partially modified"));
				break;
			}
		}
		// 성공이면 소멸자가 기록을 닫는다
	}

	// 4-a. 실패: 기존 에셋은 위에서 되돌렸고, 새 에셋은 지운다
	if (!bApplied)
	{
		for (UObject* Asset : Created)
		{
			DiscardCreatedAsset(Asset);
		}
		return false;
	}

	// 4-b. 성공: 새 에셋을 에디터에 알린다. 저장은 사용자가 한다
	for (UObject* Asset : Created)
	{
		Asset->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Asset);
	}

	return true;
}

// 계획 → 연습 → 실전
bool FDataBridgePipeline::Import(const FDataBridgeObject& Root, FDataBridgeContext& Ctx,
	FDataBridgeImportPlan* OutPlan, TArray<FString>* OutChanges)
{
	FDataBridgeImportPlan Plan;
	if (!BuildImportPlan(Root, Plan, Ctx))
	{
		return false;
	}

	const bool bOk = Rehearse(Plan, Ctx, OutChanges) && Commit(Plan, Ctx);

	if (OutPlan)
	{
		*OutPlan = MoveTemp(Plan);
	}
	return bOk;
}
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "DataBridgeImportPlan.h"
#include "DataBridgePipeline.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeTransaction.h"
#include "DataBridgeTypes.h"
#include "Editor.h"
#include "Editor/Transactor.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Misc/PackageName.h"
#include "Tests/DataBridgeTestFixtures.h"
#include "Tests/DataBridgeTestTypes.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace DataBridgeCommitTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	const TCHAR* ImportTitle = TEXT("DataBridge Import");

	// /Temp에 "Value = 1, 자식 하나(Value = 1)"인 기존 에셋
	UDataBridgeTestAsset* MakeExisting(const TCHAR* Prefix, FString& OutPackageName)
	{
		OutPackageName = DataBridgeTestFixtures::UniqueTempPackageName(TEXT("DataBridgeCommitTest"), Prefix);
		UPackage* Package = CreatePackage(*OutPackageName);
		UDataBridgeTestAsset* Asset = NewObject<UDataBridgeTestAsset>(Package, *FPackageName::GetShortName(OutPackageName), RF_Public | RF_Transactional);
		Asset->Value = 1;
		UDataBridgeTestChild* Child = NewObject<UDataBridgeTestChild>(Asset, NAME_None, RF_Transactional);
		Child->Value = 1;
		Asset->Children.Add(Child);
		return Asset;
	}

	// 기존 에셋: Value 5 + 자식 교체(9) / 새 에셋: Value 7 + 자식(4)
	FString MakeDocument(const FString& ExistingPath, const FString& NewPath)
	{
		FString Text = TEXT(R"({
			"format": "databridge/1",
			"assets": [
				{ "path": "{EXISTING}", "class": "DataBridgeTestAsset",
				  "data": { "Value": 5, "Children": [ { "$class": "DataBridgeTestChild", "Value": 9 } ] } },
				{ "path": "{NEW}", "class": "DataBridgeTestAsset",
				  "data": { "Value": 7, "Children": [ { "$class": "DataBridgeTestChild", "Value": 4 } ] } }
			]
		})");
		Text.ReplaceInline(TEXT("{EXISTING}"), *ExistingPath);
		Text.ReplaceInline(TEXT("{NEW}"), *NewPath);
		return Text;
	}

	bool PlanAndRehearse(const FString& Text, FDataBridgeImportPlan& OutPlan, FDataBridgeContext& Ctx)
	{
		TSharedPtr<FDataBridgeObject> Root;
		if (const IDataBridgeFormat* Json = FDataBridgeRegistry::FindFormatByExtension(TEXT("json")))
		{
			Json->Read(Text, Root, Ctx);
		}
		return Root.IsValid()
			&& FDataBridgePipeline::BuildImportPlan(*Root, OutPlan, Ctx)
			&& FDataBridgePipeline::Rehearse(OutPlan, Ctx);
	}

	FString ObjectPathOf(const FString& PackageName)
	{
		return PackageName + TEXT(".") + FPackageName::GetShortName(PackageName);
	}

	FTransactionContext TopUndo()
	{
		return (GEditor && GEditor->Trans) ? GEditor->Trans->GetUndoContext(true) : FTransactionContext();
	}

	// 테스트가 남긴 Import 기록을 치운다 (사용자의 Undo History를 더럽히지 않게)
	void UndoTopImportIfAny()
	{
		if (GEditor && TopUndo().Title.ToString() == ImportTitle)
		{
			GEditor->UndoTransaction(/*bCanRedo*/ false);
		}
	}

	// 테스트가 만든 새 에셋 정리
	void DiscardAsset(UObject* Asset)
	{
		if (Asset)
		{
			FAssetRegistryModule::AssetDeleted(Asset);
			Asset->ClearFlags(RF_Public | RF_Standalone);
			Asset->MarkAsGarbage();
		}
	}
}

// 1. 적용 후 실패: 기존 에셋은 원래대로(자식 인스턴스까지), 새 에셋은 사라지고, 사용자의 이전 기록은 그대로
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeCommitRollbackTest,
	"DataBridge.Commit.FailureRollsBackOnlyItsOwnChanges", DataBridgeCommitTest::TestFlags)

	bool FDataBridgeCommitRollbackTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeCommitTest;

	// 사용자의 직전 작업 흉내: 다른 에셋을 2로 바꾼 기록
	FString SentinelPath;
	TStrongObjectPtr<UDataBridgeTestAsset> SentinelAsset(MakeExisting(TEXT("Sentinel"), SentinelPath));

	FDataBridgeContext SentinelCtx;
	if (!TestTrue(TEXT("Can open sentinel transaction"), FDataBridgeTransaction::CanBegin(SentinelCtx)))
	{
		return false;
	}
	FDataBridgeTransaction Sentinel(NSLOCTEXT("DataBridgeTest", "Sentinel", "DataBridge Test Sentinel"));
	SentinelAsset->Modify();
	SentinelAsset->Value = 2;
	Sentinel.End();
	const FGuid SentinelId = Sentinel.GetTransactionId();

	FString ExistingPath;
	TStrongObjectPtr<UDataBridgeTestAsset> Existing(MakeExisting(TEXT("Existing"), ExistingPath));
	UDataBridgeTestChild* OriginalChild = Existing->Children[0];
	const FString NewPath = DataBridgeTestFixtures::UniqueTempPackageName(TEXT("DataBridgeCommitTest"), TEXT("New"));

	FDataBridgeContext Ctx;
	FDataBridgeImportPlan Plan;
	if (TestTrue(TEXT("Planned and rehearsed"), PlanAndRehearse(MakeDocument(ExistingPath, NewPath), Plan, Ctx)))
	{
		FDataBridgePipeline::bFailNextCommitForTest = true;
		const bool bCommitted = FDataBridgePipeline::Commit(Plan, Ctx);
		AddInfo(Ctx.BuildReport());

		TestFalse(TEXT("Commit failed"), bCommitted);
		TestFalse(TEXT("Failure flag was consumed"), FDataBridgePipeline::bFailNextCommitForTest);
		TestTrue(TEXT("Injected error reported"), Ctx.GetIssues().ContainsByPredicate([](const FDataBridgeIssue& Issue)
			{
				return Issue.Severity == EDataBridgeSeverity::Error && Issue.Message.Contains(TEXT("Injected"));
			}));

		// 기존 에셋: 값과 자식 인스턴스까지 원래대로
		TestEqual(TEXT("Existing value restored"), Existing->Value, 1);
		TestTrue(TEXT("Existing keeps its original child instance"), Existing->Children.Num() == 1 && Existing->Children[0] == OriginalChild);
		TestEqual(TEXT("Original child value intact"), OriginalChild->Value, 1);

		// 새 에셋: 그 경로에 없어야 한다
		TestNull(TEXT("New asset was removed"), FindObject<UObject>(nullptr, *ObjectPathOf(NewPath)));

		// 사용자의 이전 기록: 건드리지 않았어야 한다
		TestTrue(TEXT("User's previous undo record is still on top"), TopUndo().TransactionId == SentinelId);
		TestEqual(TEXT("User's previous change is intact"), SentinelAsset->Value, 2);
	}
	else
	{
		AddError(Ctx.BuildReport());
	}

	Sentinel.UndoIfOwned();   // 센티널 기록 정리
	return true;
}

// 2. 성공: 기존 에셋은 바뀌고, 새 에셋은 진짜 에셋으로 생기고, Undo 기록이 하나 생긴다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeCommitSuccessTest,
	"DataBridge.Commit.AppliesAsOneUndoRecord", DataBridgeCommitTest::TestFlags)

	bool FDataBridgeCommitSuccessTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeCommitTest;

	FString ExistingPath;
	TStrongObjectPtr<UDataBridgeTestAsset> Existing(MakeExisting(TEXT("Existing"), ExistingPath));
	UDataBridgeTestChild* OriginalChild = Existing->Children[0];
	const FString NewPath = DataBridgeTestFixtures::UniqueTempPackageName(TEXT("DataBridgeCommitTest"), TEXT("New"));
	const FGuid TopBefore = TopUndo().TransactionId;

	FDataBridgeContext Ctx;
	FDataBridgeImportPlan Plan;
	if (!TestTrue(TEXT("Planned and rehearsed"), PlanAndRehearse(MakeDocument(ExistingPath, NewPath), Plan, Ctx)))
	{
		AddError(Ctx.BuildReport());
		return false;
	}
	if (!TestTrue(TEXT("Commit succeeded"), FDataBridgePipeline::Commit(Plan, Ctx)))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	// 기존 에셋
	TestEqual(TEXT("Existing value applied"), Existing->Value, 5);
	TestTrue(TEXT("Existing child replaced with a new instance"),
		Existing->Children.Num() == 1 && Existing->Children[0] != OriginalChild && Existing->Children[0]->Value == 9);
	TestTrue(TEXT("Existing package is dirty (not saved)"), Existing->GetPackage()->IsDirty());

	// 새 에셋
	UDataBridgeTestAsset* Created = FindObject<UDataBridgeTestAsset>(nullptr, *ObjectPathOf(NewPath));
	if (TestNotNull(TEXT("New asset created"), Created))
	{
		TestEqual(TEXT("New asset value"), Created->Value, 7);
		TestTrue(TEXT("New asset is a real asset (Public | Standalone)"), Created->HasAllFlags(RF_Public | RF_Standalone));
		TestTrue(TEXT("New asset child"), Created->Children.Num() == 1 && Created->Children[0]->Value == 4);
		TestTrue(TEXT("New package is dirty (not saved)"), Created->GetPackage()->IsDirty());
	}

	// Undo 기록
	const FTransactionContext Top = TopUndo();
	TestTrue(TEXT("Import added one record on top of the undo history"),
		Top.TransactionId != TopBefore && Top.Title.ToString() == ImportTitle);

	UndoTopImportIfAny();
	DiscardAsset(Created);
	return true;
}

// 3. 성공 후 Ctrl+Z: 기존 에셋은 원래대로, 새 에셋과 그 자식은 멀쩡히 남는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeCommitUndoTest,
	"DataBridge.Commit.UndoRestoresUpdatesAndKeepsCreated", DataBridgeCommitTest::TestFlags)

	bool FDataBridgeCommitUndoTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeCommitTest;

	FString ExistingPath;
	TStrongObjectPtr<UDataBridgeTestAsset> Existing(MakeExisting(TEXT("Existing"), ExistingPath));
	UDataBridgeTestChild* OriginalChild = Existing->Children[0];
	const FString NewPath = DataBridgeTestFixtures::UniqueTempPackageName(TEXT("DataBridgeCommitTest"), TEXT("New"));

	FDataBridgeContext Ctx;
	FDataBridgeImportPlan Plan;
	if (!TestTrue(TEXT("Planned and rehearsed"), PlanAndRehearse(MakeDocument(ExistingPath, NewPath), Plan, Ctx))
		|| !TestTrue(TEXT("Commit succeeded"), FDataBridgePipeline::Commit(Plan, Ctx)))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	UDataBridgeTestAsset* Created = FindObject<UDataBridgeTestAsset>(nullptr, *ObjectPathOf(NewPath));

	// 사용자가 Ctrl+Z
	if (!TestTrue(TEXT("Top undo record is the import"), TopUndo().Title.ToString() == ImportTitle)
		|| !TestTrue(TEXT("Undo succeeded"), GEditor->UndoTransaction(/*bCanRedo*/ false)))
	{
		DiscardAsset(Created);
		return false;
	}

	// 기존 에셋: 원래대로
	TestEqual(TEXT("Existing value restored"), Existing->Value, 1);
	TestTrue(TEXT("Existing keeps its original child instance"), Existing->Children.Num() == 1 && Existing->Children[0] == OriginalChild);
	TestEqual(TEXT("Original child value intact"), OriginalChild->Value, 1);

	// 새 에셋: Undo 대상이 아니므로 그대로 (자식이 Garbage가 되면 안 된다)
	if (TestNotNull(TEXT("New asset survives undo"), Created))
	{
		TestTrue(TEXT("New asset is not garbage"), IsValid(Created));
		TestEqual(TEXT("New asset value kept"), Created->Value, 7);
		TestTrue(TEXT("New asset's child survives undo"),
			Created->Children.Num() == 1 && IsValid(Created->Children[0]) && Created->Children[0]->Value == 4);
	}

	DiscardAsset(Created);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DataBridgeImportPlan.h"
#include "DataBridgePipeline.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Misc/PackageName.h"
#include "Tests/DataBridgeTestFixtures.h"
#include "Tests/DataBridgeTestTypes.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace DataBridgeRehearseTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	// /Temp에 "Value = 1, 자식 하나(Value = 1)"인 기존 에셋
	UDataBridgeTestAsset* MakeExisting(FString& OutPackageName)
	{
		OutPackageName = DataBridgeTestFixtures::UniqueTempPackageName(TEXT("DataBridgeRehearseTest"), TEXT("Existing"));
		UPackage* Package = CreatePackage(*OutPackageName);
		UDataBridgeTestAsset* Asset = NewObject<UDataBridgeTestAsset>(Package, *FPackageName::GetShortName(OutPackageName), RF_Public | RF_Transactional);
		Asset->Value = 1;
		UDataBridgeTestChild* Child = NewObject<UDataBridgeTestChild>(Asset, NAME_None, RF_Transactional);
		Child->Value = 1;
		Asset->Children.Add(Child);
		return Asset;
	}

	bool ParseAndPlan(const FString& Text, FDataBridgeImportPlan& OutPlan, FDataBridgeContext& Ctx)
	{
		TSharedPtr<FDataBridgeObject> Root;
		if (const IDataBridgeFormat* Json = FDataBridgeRegistry::FindFormatByExtension(TEXT("json")))
		{
			Json->Read(Text, Root, Ctx);
		}
		return Root.IsValid() && FDataBridgePipeline::BuildImportPlan(*Root, OutPlan, Ctx);
	}

	bool HasIssue(const FDataBridgeContext& Ctx, const FString& Path, const FString& MessagePart)
	{
		return Ctx.GetIssues().ContainsByPredicate([&](const FDataBridgeIssue& Issue)
			{
				return Issue.Severity == EDataBridgeSeverity::Error && Issue.Path == Path && Issue.Message.Contains(MessagePart);
			});
	}
}

// 1. 성공하는 연습: 변경 목록이 나오고, 진짜 에셋은 그대로, 새 에셋은 생기지 않는다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeRehearseSuccessTest,
	"DataBridge.Rehearse.ReportsChangesWithoutTouchingAssets", DataBridgeRehearseTest::TestFlags)

	bool FDataBridgeRehearseSuccessTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeRehearseTest;

	FString ExistingPath;
	TStrongObjectPtr<UDataBridgeTestAsset> Existing(MakeExisting(ExistingPath));
	UDataBridgeTestChild* OriginalChild = Existing->Children[0];
	const FString NewPath = DataBridgeTestFixtures::UniqueTempPackageName(TEXT("DataBridgeRehearseTest"), TEXT("New"));

	FString Text = TEXT(R"({
		"format": "databridge/1",
		"assets": [
			{ "path": "{EXISTING}", "class": "DataBridgeTestAsset",
			  "data": { "Value": 5, "Children": [ { "$class": "DataBridgeTestChild", "Value": 9 } ] } },
			{ "path": "{NEW}", "class": "DataBridgeTestAsset", "data": { "Value": 7 } }
		]
	})");
	Text.ReplaceInline(TEXT("{EXISTING}"), *ExistingPath);
	Text.ReplaceInline(TEXT("{NEW}"), *NewPath);

	FDataBridgeContext Ctx;
	FDataBridgeImportPlan Plan;
	if (!TestTrue(TEXT("Planned"), ParseAndPlan(Text, Plan, Ctx)))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	TArray<FString> Changes;
	const bool bRehearsed = FDataBridgePipeline::Rehearse(Plan, Ctx, &Changes);
	AddInfo(FString::Join(Changes, TEXT("\n")));
	if (!TestTrue(TEXT("Rehearsal succeeded"), bRehearsed))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	TestTrue(TEXT("Change: Value 1 -> 5"), Changes.Contains(TEXT("assets[0].data.Value: 1 -> 5")));
	TestTrue(TEXT("Change: child value 1 -> 9"), Changes.Contains(TEXT("assets[0].data.Children[0].Value: 1 -> 9")));
	TestTrue(TEXT("Change: new asset listed"), Changes.ContainsByPredicate([](const FString& Change) { return Change.EndsWith(TEXT(": new asset")); }));

	// 진짜는 그대로
	TestEqual(TEXT("Real asset value unchanged"), Existing->Value, 1);
	TestTrue(TEXT("Real asset keeps its original child"), Existing->Children.Num() == 1 && Existing->Children[0] == OriginalChild);
	TestEqual(TEXT("Real child value unchanged"), OriginalChild->Value, 1);
	TestNull(TEXT("New asset was not created"), FindObject<UObject>(nullptr, *(NewPath + TEXT(".") + FPackageName::GetShortName(NewPath))));

	return true;
}

// 2. 실패하는 연습: 정확한 경로로 보고되고, 진짜는 그대로
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeRehearseFailureTest,
	"DataBridge.Rehearse.FailureLeavesAssetsUntouched", DataBridgeRehearseTest::TestFlags)

	bool FDataBridgeRehearseFailureTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeRehearseTest;

	FString ExistingPath;
	TStrongObjectPtr<UDataBridgeTestAsset> Existing(MakeExisting(ExistingPath));
	UDataBridgeTestChild* OriginalChild = Existing->Children[0];

	// Value와 Children은 정상, Speedd에서 실패 → 앞의 변경도 진짜에는 하나도 반영되면 안 된다
	FString Text = TEXT(R"({
		"format": "databridge/1",
		"assets": [
			{ "path": "{EXISTING}", "class": "DataBridgeTestAsset",
			  "data": { "Value": 5, "Children": [ { "$class": "DataBridgeTestChild", "Value": 9 } ], "Speedd": 1 } }
		]
	})");
	Text.ReplaceInline(TEXT("{EXISTING}"), *ExistingPath);

	FDataBridgeContext Ctx;
	FDataBridgeImportPlan Plan;
	if (!TestTrue(TEXT("Planned (plan does not inspect data)"), ParseAndPlan(Text, Plan, Ctx)))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	const bool bRehearsed = FDataBridgePipeline::Rehearse(Plan, Ctx);
	AddInfo(Ctx.BuildReport());

	TestFalse(TEXT("Rehearsal failed"), bRehearsed);
	TestTrue(TEXT("Error at assets[0].data.Speedd"), HasIssue(Ctx, TEXT("assets[0].data.Speedd"), TEXT("Unknown field")));

	TestEqual(TEXT("Real asset value unchanged"), Existing->Value, 1);
	TestTrue(TEXT("Real asset keeps its original child"), Existing->Children.Num() == 1 && Existing->Children[0] == OriginalChild);
	TestEqual(TEXT("Real child value unchanged"), OriginalChild->Value, 1);

	return true;
}

// 3. 같은 문서 안의 새 에셋끼리 참조: 연습에서는 복제본끼리 연결되어야 한다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeRehearseCrossReferenceTest,
	"DataBridge.Rehearse.ResolvesReferencesBetweenNewAssets", DataBridgeRehearseTest::TestFlags)

	bool FDataBridgeRehearseCrossReferenceTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeRehearseTest;

	const FString HolderPath = DataBridgeTestFixtures::UniqueTempPackageName(TEXT("DataBridgeRehearseTest"), TEXT("Holder"));
	const FString TargetPath = DataBridgeTestFixtures::UniqueTempPackageName(TEXT("DataBridgeRehearseTest"), TEXT("Target"));

	// Holder(새 에셋)가 Target(같은 문서의 새 에셋)을 하드 참조한다. 둘 다 아직 디스크에도 메모리에도 없다.
	FString Text = TEXT(R"({
		"format": "databridge/1",
		"assets": [
			{ "path": "{HOLDER}", "class": "DataBridgeTestAllTypes", "data": { "HardRef": "{TARGET}" } },
			{ "path": "{TARGET}", "class": "DataBridgeTestAsset", "data": { "Value": 3 } }
		]
	})");
	Text.ReplaceInline(TEXT("{HOLDER}"), *HolderPath);
	Text.ReplaceInline(TEXT("{TARGET}"), *TargetPath);

	FDataBridgeContext Ctx;
	FDataBridgeImportPlan Plan;
	if (!TestTrue(TEXT("Planned"), ParseAndPlan(Text, Plan, Ctx)))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	const bool bRehearsed = FDataBridgePipeline::Rehearse(Plan, Ctx);
	if (!TestTrue(TEXT("Reference to a new asset in the same document resolves"), bRehearsed))
	{
		AddError(Ctx.BuildReport());
	}
	TestFalse(TEXT("No errors"), Ctx.HasErrors());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
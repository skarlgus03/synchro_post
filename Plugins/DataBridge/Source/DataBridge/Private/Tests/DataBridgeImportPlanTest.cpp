#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DataBridgeImportPlan.h"
#include "DataBridgePipeline.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Misc/PackageName.h"
#include "Tests/DataBridgeTestTypes.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace DataBridgePlanTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FString UniqueTempPackage(const TCHAR* Prefix)
	{
		return FString::Printf(TEXT("/Temp/DataBridgePlanTest/%s_%s"), Prefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	// 에셋 이름 = 패키지 짧은 이름 (일반 에셋 규칙). 메모리에만 존재, 저장하지 않음.
	UDataBridgeTestAsset* MakeExistingAsset(const FString& PackageName)
	{
		UPackage* Package = CreatePackage(*PackageName);
		return NewObject<UDataBridgeTestAsset>(Package, *FPackageName::GetShortName(PackageName), RF_Transactional);
	}

	// 창구로 JSON 포맷을 찾아 읽는다 (외부 사용자와 같은 경로)
	TSharedPtr<FDataBridgeObject> ParseJson(const FString& Text, FDataBridgeContext& Ctx)
	{
		TSharedPtr<FDataBridgeObject> Root;
		if (const IDataBridgeFormat* Json = FDataBridgeRegistry::FindFormatByExtension(TEXT("json")))
		{
			Json->Read(Text, Root, Ctx);
		}
		return Root;
	}

	bool HasError(const FDataBridgeContext& Ctx, const FString& Path, const FString& MessagePart)
	{
		return Ctx.GetIssues().ContainsByPredicate([&](const FDataBridgeIssue& Issue)
			{
				return Issue.Severity == EDataBridgeSeverity::Error
					&& Issue.Path == Path
					&& Issue.Message.Contains(MessagePart);
			});
	}
}

// 정상 문서: 기존 에셋은 Update, 없는 경로는 Create, 짧은 이름/전체 경로 모두 해석
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeImportPlanValidTest,
	"DataBridge.ImportPlan.ValidDocument", DataBridgePlanTest::TestFlags)

	bool FDataBridgeImportPlanValidTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgePlanTest;

	const FString ExistingPath = UniqueTempPackage(TEXT("Existing"));
	const FString NewPath = UniqueTempPackage(TEXT("New"));
	TStrongObjectPtr<UDataBridgeTestAsset> Existing(MakeExistingAsset(ExistingPath));

	FString Text = TEXT(R"({
		"format": "databridge/1",
		"assets": [
			{ "path": "{EXISTING}", "class": "DataBridgeTestAsset", "data": { "Value": 5 } },
			{ "path": "{NEW}", "class": "/Script/DataBridge.DataBridgeTestAsset", "data": {} }
		]
	})");
	Text.ReplaceInline(TEXT("{EXISTING}"), *ExistingPath);
	Text.ReplaceInline(TEXT("{NEW}"), *NewPath);

	FDataBridgeContext Ctx;
	const TSharedPtr<FDataBridgeObject> Root = ParseJson(Text, Ctx);
	if (!TestTrue(TEXT("JSON parsed"), Root.IsValid()))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	FDataBridgeImportPlan Plan;
	const bool bBuilt = FDataBridgePipeline::BuildImportPlan(*Root, Plan, Ctx);
	AddInfo(Plan.Describe());
	if (!TestTrue(TEXT("Plan built"), bBuilt))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	TestEqual(TEXT("Entry count"), Plan.Entries.Num(), 2);
	TestEqual(TEXT("Update count"), Plan.Num(EDataBridgeImportAction::Update), 1);
	TestEqual(TEXT("Create count"), Plan.Num(EDataBridgeImportAction::Create), 1);

	if (const FDataBridgeImportEntry* Entry = Plan.FindByPackageName(ExistingPath))
	{
		TestTrue(TEXT("Existing asset -> Update"), Entry->Action == EDataBridgeImportAction::Update);
		TestTrue(TEXT("Short class name resolved"), Entry->Class.Get() == UDataBridgeTestAsset::StaticClass());
		TestTrue(TEXT("Data object kept"), Entry->Data.IsValid());
		TestEqual(TEXT("SourceIndex of existing"), Entry->SourceIndex, 0);
	}
	else
	{
		AddError(TEXT("Entry for the existing asset is missing"));
	}

	if (const FDataBridgeImportEntry* Entry = Plan.FindByPackageName(NewPath))
	{
		TestTrue(TEXT("Missing path -> Create"), Entry->Action == EDataBridgeImportAction::Create);
		TestTrue(TEXT("Full class path resolved"), Entry->Class.Get() == UDataBridgeTestAsset::StaticClass());
		TestEqual(TEXT("Asset name = package short name"), Entry->AssetName, FPackageName::GetShortName(NewPath));
		TestEqual(TEXT("SourceIndex of new"), Entry->SourceIndex, 1);
	}
	else
	{
		AddError(TEXT("Entry for the new asset is missing"));
	}

	TestFalse(TEXT("No errors"), Ctx.HasErrors());
	return true;
}

// 틀린 문서: 모든 에러를 한 번에, 정확한 IR 경로로 보고하고 계획은 비운다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeImportPlanErrorsTest,
	"DataBridge.ImportPlan.ReportsAllErrorsWithPaths", DataBridgePlanTest::TestFlags)

	bool FDataBridgeImportPlanErrorsTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgePlanTest;

	const FString ExistingPath = UniqueTempPackage(TEXT("Existing"));
	TStrongObjectPtr<UDataBridgeTestAsset> Existing(MakeExistingAsset(ExistingPath));

	FString Text = TEXT(R"({
		"format": "databridge/2",
		"assets": [
			{ "path": "/Game/DataBridgePlanTest/DA_A", "class": "NoSuchClassXYZ", "data": {} },
			{ "path": "/Game/DataBridgePlanTest/DA_B", "class": "DataBridgeTestAsset" },
			{ "path": "/Game/DataBridgePlanTest/DA_A", "class": "DataBridgeTestAsset", "data": {} },
			{ "path": "/Script/Engine/Foo", "class": "DataBridgeTestAsset", "data": {} },
			{ "path": "/Game/DataBridgePlanTest/DA_C.DA_D", "class": "DataBridgeTestAsset", "data": {} },
			{ "path": "{EXISTING}", "class": "DataBridgeTestChild", "data": {} },
			{ "path": "/Game/DataBridgePlanTest/DA_E", "class": "DataBridgeTestAbstractAsset", "data": {} },
			{ "path": "/Game/DataBridgePlanTest/DA_F", "class": "/Script/Engine.Actor", "data": {} }
		]
	})");
	Text.ReplaceInline(TEXT("{EXISTING}"), *ExistingPath);

	FDataBridgeContext Ctx;
	const TSharedPtr<FDataBridgeObject> Root = ParseJson(Text, Ctx);
	if (!TestTrue(TEXT("JSON parsed"), Root.IsValid()))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	FDataBridgeImportPlan Plan;
	const bool bBuilt = FDataBridgePipeline::BuildImportPlan(*Root, Plan, Ctx);
	AddInfo(Ctx.BuildReport());   // 실제 보고서를 로그에 남겨 눈으로도 확인

	TestFalse(TEXT("Plan rejected"), bBuilt);
	TestEqual(TEXT("Plan is empty on failure"), Plan.Entries.Num(), 0);

	TestTrue(TEXT("format: version"), HasError(Ctx, TEXT("format"), TEXT("Unsupported format version")));
	TestTrue(TEXT("[0] class not found"), HasError(Ctx, TEXT("assets[0].class"), TEXT("not found")));
	TestTrue(TEXT("[1] data missing"), HasError(Ctx, TEXT("assets[1].data"), TEXT("Missing required field")));
	TestTrue(TEXT("[2] duplicate path"), HasError(Ctx, TEXT("assets[2].path"), TEXT("Duplicate path")));
	TestTrue(TEXT("[3] forbidden root"), HasError(Ctx, TEXT("assets[3].path"), TEXT("not allowed")));
	TestTrue(TEXT("[4] object name mismatch"), HasError(Ctx, TEXT("assets[4].path"), TEXT("must match")));
	TestTrue(TEXT("[5] class change"), HasError(Ctx, TEXT("assets[5].class"), TEXT("Changing an asset's class")));
	TestTrue(TEXT("[6] abstract class"), HasError(Ctx, TEXT("assets[6].class"), TEXT("abstract")));
	TestTrue(TEXT("[7] actor class"), HasError(Ctx, TEXT("assets[7].class"), TEXT("Actor classes")));

	TestEqual(TEXT("No unexpected errors"), Ctx.GetErrorCount(), 9);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
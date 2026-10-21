#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Handlers/DataBridgeExportWalker.h"
#include "Handlers/DataBridgeImportWalker.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Tests/DataBridgeTestFixtures.h"
#include "Tests/DataBridgeTestTypes.h"
#include "UObject/StrongObjectPtr.h"

namespace DataBridgeBitmaskTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	TSharedPtr<FDataBridgeObject> Export(const UObject* Object, FDataBridgeContext& Ctx)
	{
		const FDataBridgeReferenceResolver Resolver;
		FDataBridgeExportWalker Exporter(Resolver, Ctx);
		return Exporter.ExportObject(Object);
	}

	bool Import(const FString& Text, UObject* Target, FDataBridgeContext& Ctx)
	{
		TSharedPtr<FDataBridgeObject> Data;
		if (const IDataBridgeFormat* Json = FDataBridgeRegistry::FindFormatByExtension(TEXT("json")))
		{
			Json->Read(Text, Data, Ctx);
		}
		if (!Data.IsValid())
		{
			return false;
		}
		const FDataBridgeReferenceResolver Resolver;
		FDataBridgeImportWalker Importer(Resolver, Ctx);
		return Importer.ImportObject(*Data, Target);
	}

	// ["A", "B"] → "A,B" (비교용). 배열이 아니면 "(not an array)"
	FString JoinNames(const TSharedPtr<FDataBridgeObject>& Data, const TCHAR* Field)
	{
		const TArray<TSharedPtr<FDataBridgeValue>>* Items = nullptr;
		if (!Data.IsValid() || !Data->TryGetArrayField(Field, Items))
		{
			return TEXT("(not an array)");
		}
		TArray<FString> Names;
		for (const TSharedPtr<FDataBridgeValue>& Item : *Items)
		{
			Names.Add(Item->AsString());
		}
		return FString::Join(Names, TEXT(","));
	}

	bool HasIssue(const FDataBridgeContext& Ctx, EDataBridgeSeverity Severity, const TCHAR* Path, const TCHAR* MessagePart)
	{
		return Ctx.GetIssues().ContainsByPredicate([&](const FDataBridgeIssue& Issue)
			{
				return Issue.Severity == Severity && Issue.Path == Path && Issue.Message.Contains(MessagePart);
			});
	}
}

// 1. Export: 두 방식 모두 이름 배열로, 0은 빈 배열로, 이름 없는 비트가 있으면 숫자 + 경고
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeBitmaskExportTest,
	"DataBridge.Bitmask.ExportsFlagNames", DataBridgeBitmaskTest::TestFlags)

	bool FDataBridgeBitmaskExportTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeBitmaskTest;

	TStrongObjectPtr<UDataBridgeTestAllTypes> Obj(DataBridgeTestFixtures::MakeEmptyAllTypes(TEXT("BitmaskExport")));

	// 이름 있는 비트만
	{
		Obj->MaskFlags = 1 | 8;                   // Red | Blue
		Obj->BitIndexFlags = (1 << 0) | (1 << 2); // First | Third

		FDataBridgeContext Ctx;
		const TSharedPtr<FDataBridgeObject> Data = Export(Obj.Get(), Ctx);
		TestEqual(TEXT("Value-as-mask enum -> names"), JoinNames(Data, TEXT("MaskFlags")), FString(TEXT("Red,Blue")));
		TestEqual(TEXT("Bit-index enum -> names"), JoinNames(Data, TEXT("BitIndexFlags")), FString(TEXT("First,Third")));
		TestFalse(TEXT("No issues"), Ctx.GetIssues().Num() > 0);
	}

	// 0 → []
	{
		Obj->MaskFlags = 0;

		FDataBridgeContext Ctx;
		const TSharedPtr<FDataBridgeObject> Data = Export(Obj.Get(), Ctx);
		TestEqual(TEXT("Zero -> empty array"), JoinNames(Data, TEXT("MaskFlags")), FString());
	}

	// 이름 없는 비트(4) → 숫자 그대로 + 경고
	{
		Obj->MaskFlags = 1 | 4;

		FDataBridgeContext Ctx;
		const TSharedPtr<FDataBridgeObject> Data = Export(Obj.Get(), Ctx);
		double Number = 0.0;
		TestTrue(TEXT("Unnamed bit -> exported as a number"), Data.IsValid() && Data->TryGetNumberField(TEXT("MaskFlags"), Number) && Number == 5.0);
		TestTrue(TEXT("Unnamed bit -> warning"), HasIssue(Ctx, EDataBridgeSeverity::Warning, TEXT("MaskFlags"), TEXT("does not name")));
		TestFalse(TEXT("Unnamed bit is not an error"), Ctx.HasErrors());
	}

	return true;
}

// 2. Import: 이름(대소문자 무시, 전체 이름 허용), 숫자, 빈 배열
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeBitmaskImportTest,
	"DataBridge.Bitmask.ImportsNamesAndNumbers", DataBridgeBitmaskTest::TestFlags)

	bool FDataBridgeBitmaskImportTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeBitmaskTest;

	TStrongObjectPtr<UDataBridgeTestAllTypes> Obj(DataBridgeTestFixtures::MakeEmptyAllTypes(TEXT("BitmaskImport")));

	{
		FDataBridgeContext Ctx;
		const bool bImported = Import(TEXT(R"({
			"MaskFlags": [ "red", "EDataBridgeTestMaskFlags::Blue" ],
			"BitIndexFlags": 6
		})"), Obj.Get(), Ctx);

		if (!TestTrue(TEXT("Imported"), bImported))
		{
			AddError(Ctx.BuildReport());
		}
		TestEqual(TEXT("Names (case-insensitive, full name) -> bits"), Obj->MaskFlags, 1 | 8);
		TestEqual(TEXT("Number still accepted"), Obj->BitIndexFlags, 6);
	}

	{
		FDataBridgeContext Ctx;
		TestTrue(TEXT("Empty array imported"), Import(TEXT(R"({ "MaskFlags": [] })"), Obj.Get(), Ctx));
		TestEqual(TEXT("Empty array -> 0"), Obj->MaskFlags, 0);
	}

	return true;
}

// 3. Import 에러: 모르는 이름, 중복(전체 이름과 짧은 이름이 같은 비트), 잘못된 타입
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeBitmaskErrorsTest,
	"DataBridge.Bitmask.ImportReportsErrors", DataBridgeBitmaskTest::TestFlags)

	bool FDataBridgeBitmaskErrorsTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeBitmaskTest;

	TStrongObjectPtr<UDataBridgeTestAllTypes> Obj(DataBridgeTestFixtures::MakeEmptyAllTypes(TEXT("BitmaskErrors")));

	FDataBridgeContext Ctx;
	const bool bImported = Import(TEXT(R"({
		"MaskFlags": [ "Red", "Purple", "EDataBridgeTestMaskFlags::Red" ],
		"BitIndexFlags": "First"
	})"), Obj.Get(), Ctx);
	AddInfo(Ctx.BuildReport());

	TestFalse(TEXT("Import rejected"), bImported);
	TestTrue(TEXT("MaskFlags[1]: unknown flag"), HasIssue(Ctx, EDataBridgeSeverity::Error, TEXT("MaskFlags[1]"), TEXT("is not a flag")));
	TestTrue(TEXT("MaskFlags[1]: lists the valid names"), HasIssue(Ctx, EDataBridgeSeverity::Error, TEXT("MaskFlags[1]"), TEXT("Red, Green, Blue")));
	TestTrue(TEXT("MaskFlags[2]: duplicate by bit"), HasIssue(Ctx, EDataBridgeSeverity::Error, TEXT("MaskFlags[2]"), TEXT("Duplicate flag")));
	TestTrue(TEXT("BitIndexFlags: wrong type"), HasIssue(Ctx, EDataBridgeSeverity::Error, TEXT("BitIndexFlags"), TEXT("Expected an array")));
	TestEqual(TEXT("No unexpected errors"), Ctx.GetErrorCount(), 3);

	// 실패한 필드는 값을 쓰지 않는다
	TestEqual(TEXT("MaskFlags untouched"), Obj->MaskFlags, 0);
	TestEqual(TEXT("BitIndexFlags untouched"), Obj->BitIndexFlags, 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
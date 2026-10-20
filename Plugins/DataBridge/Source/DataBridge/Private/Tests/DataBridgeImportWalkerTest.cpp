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

namespace DataBridgeImportTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	// 두 IR이 같은지. 숫자는 허용 오차로 비교하고, 다른 곳을 경로와 함께 모은다.
	void CollectDiffs(const TSharedPtr<FDataBridgeValue>& A, const TSharedPtr<FDataBridgeValue>& B, const FString& Path, TArray<FString>& OutDiffs)
	{
		if (!A.IsValid() || !B.IsValid())
		{
			if (A.IsValid() != B.IsValid())
			{
				OutDiffs.Add(Path + TEXT(": present on one side only"));
			}
			return;
		}
		if (A->Type != B->Type)
		{
			OutDiffs.Add(Path + TEXT(": type differs"));
			return;
		}

		switch (A->Type)
		{
		case EJson::Number:
			if (!FMath::IsNearlyEqual(A->AsNumber(), B->AsNumber(), 1e-3))
			{
				OutDiffs.Add(FString::Printf(TEXT("%s: %g != %g"), *Path, A->AsNumber(), B->AsNumber()));
			}
			break;
		case EJson::String:
			if (!A->AsString().Equals(B->AsString(), ESearchCase::CaseSensitive))
			{
				OutDiffs.Add(FString::Printf(TEXT("%s: '%s' != '%s'"), *Path, *A->AsString(), *B->AsString()));
			}
			break;
		case EJson::Boolean:
			if (A->AsBool() != B->AsBool())
			{
				OutDiffs.Add(Path + TEXT(": bool differs"));
			}
			break;
		case EJson::Array:
		{
			const TArray<TSharedPtr<FDataBridgeValue>>& ArrA = A->AsArray();
			const TArray<TSharedPtr<FDataBridgeValue>>& ArrB = B->AsArray();
			if (ArrA.Num() != ArrB.Num())
			{
				OutDiffs.Add(FString::Printf(TEXT("%s: %d elements != %d"), *Path, ArrA.Num(), ArrB.Num()));
				return;
			}
			for (int32 Index = 0; Index < ArrA.Num(); ++Index)
			{
				CollectDiffs(ArrA[Index], ArrB[Index], FString::Printf(TEXT("%s[%d]"), *Path, Index), OutDiffs);
			}
			break;
		}
		case EJson::Object:
		{
			const TMap<FString, TSharedPtr<FDataBridgeValue>>& MapA = A->AsObject()->Values;
			const TMap<FString, TSharedPtr<FDataBridgeValue>>& MapB = B->AsObject()->Values;
			for (const TPair<FString, TSharedPtr<FDataBridgeValue>>& Pair : MapA)
			{
				CollectDiffs(Pair.Value, MapB.FindRef(Pair.Key), Path + TEXT(".") + Pair.Key, OutDiffs);
			}
			for (const TPair<FString, TSharedPtr<FDataBridgeValue>>& Pair : MapB)
			{
				if (!MapA.Contains(Pair.Key))
				{
					OutDiffs.Add(Path + TEXT(".") + Pair.Key + TEXT(": present on one side only"));
				}
			}
			break;
		}
		default:
			break;
		}
	}

	FString ToJsonText(const TSharedPtr<FDataBridgeObject>& Object, FDataBridgeContext& Ctx)
	{
		FString Text;
		if (const IDataBridgeFormat* Json = FDataBridgeRegistry::FindFormatByExtension(TEXT("json")))
		{
			Json->Write(Object.ToSharedRef(), Text, Ctx);
		}
		return Text;
	}
}

// 왕복: 채운 오브젝트 → Export → 빈 오브젝트에 Import → Export → 처음과 같은가
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeImportRoundTripTest,
	"DataBridge.Walker.RoundTripAllTypes", DataBridgeImportTest::TestFlags)

	bool FDataBridgeImportRoundTripTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeImportTest;

	TStrongObjectPtr<UDataBridgeTestAllTypes> Source(DataBridgeTestFixtures::MakeFilledAllTypes());
	TStrongObjectPtr<UDataBridgeTestAllTypes> Dest(DataBridgeTestFixtures::MakeEmptyAllTypes(TEXT("Dest")));

	FDataBridgeContext Ctx;
	const FDataBridgeReferenceResolver Resolver;

	// 1. Source → IR
	FDataBridgeExportWalker Exporter1(Resolver, Ctx);
	const TSharedPtr<FDataBridgeObject> First = Exporter1.ExportObject(Source.Get());
	if (!TestTrue(TEXT("First export"), First.IsValid()))
	{
		AddError(Ctx.BuildReport());
		return false;
	}
	AddInfo(ToJsonText(First, Ctx));

	// 2. IR → 빈 Dest
	FDataBridgeImportWalker Importer(Resolver, Ctx);
	const bool bImported = Importer.ImportObject(*First, Dest.Get());
	if (!TestTrue(TEXT("Import succeeded"), bImported))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	// 3. Dest → IR, 그리고 비교
	FDataBridgeExportWalker Exporter2(Resolver, Ctx);
	const TSharedPtr<FDataBridgeObject> Second = Exporter2.ExportObject(Dest.Get());

	TArray<FString> Diffs;
	CollectDiffs(MakeShared<FJsonValueObject>(First), MakeShared<FJsonValueObject>(Second), TEXT("data"), Diffs);
	for (const FString& Diff : Diffs)
	{
		AddError(TEXT("Round-trip diff: ") + Diff);
	}
	TestEqual(TEXT("No round-trip differences"), Diffs.Num(), 0);
	TestEqual(TEXT("No issues"), Ctx.GetIssues().Num(), 0);

	// 4. Instanced는 새 인스턴스, 외부 참조는 같은 에셋
	if (TestNotNull(TEXT("SingleChild created"), Dest->SingleChild.Get()))
	{
		TestTrue(TEXT("SingleChild is a new instance"), Dest->SingleChild != Source->SingleChild);
		TestTrue(TEXT("SingleChild is owned by Dest"), Dest->SingleChild->GetOuter() == Dest.Get());
	}
	TestTrue(TEXT("HardRef points to the same external asset"), Dest->HardRef == Source->HardRef);
	TestEqual(TEXT("Created objects tracked (1 single + 2 array)"), Importer.GetCreatedObjects().Num(), 3);
	TestTrue(TEXT("Transform rotation survives (quat compare)"),
		Dest->Transform.GetRotation().Equals(Source->Transform.GetRotation(), 1e-4));

	return true;
}

// 틀린 값: 각 에러가 정확한 경로로 보고되는가
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeImportErrorsTest,
	"DataBridge.Walker.ImportReportsErrors", DataBridgeImportTest::TestFlags)

	bool FDataBridgeImportErrorsTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDataBridgeTestAllTypes> Dest(DataBridgeTestFixtures::MakeEmptyAllTypes(TEXT("Errors")));

	const FString Text = TEXT(R"({
		"Int": "seven",
		"Byte": 300,
		"Enum": "Delta",
		"Tag": "No.Such.Tag",
		"HardRef": "/Game/DataBridgeImportTest/DA_Missing",
		"SingleChild": { "$class": "/Script/DataBridge.DataBridgeTestAsset" },
		"Children": [ { "Value": 1 } ],
		"Speedd": 1,
		"ReadOnly": 1,
		"IntToString": [ { "key": 1, "value": "a" }, { "key": 1, "value": "b" } ]
	})");

	FDataBridgeContext Ctx;
	TSharedPtr<FDataBridgeObject> Data;
	if (const IDataBridgeFormat* Json = FDataBridgeRegistry::FindFormatByExtension(TEXT("json")))
	{
		Json->Read(Text, Data, Ctx);
	}
	if (!TestTrue(TEXT("JSON parsed"), Data.IsValid()))
	{
		AddError(Ctx.BuildReport());
		return false;
	}
	const FDataBridgeReferenceResolver Resolver;
	FDataBridgeImportWalker Importer(Resolver, Ctx);
	const bool bImported = Importer.ImportObject(*Data, Dest.Get());
	AddInfo(Ctx.BuildReport());

	TestFalse(TEXT("Import rejected"), bImported);

	auto HasError = [&Ctx](const TCHAR* Path, const TCHAR* MessagePart)
		{
			return Ctx.GetIssues().ContainsByPredicate([&](const FDataBridgeIssue& Issue)
				{
					return Issue.Severity == EDataBridgeSeverity::Error && Issue.Path == Path && Issue.Message.Contains(MessagePart);
				});
		};

	TestTrue(TEXT("Int: type"), HasError(TEXT("Int"), TEXT("Expected number")));
	TestTrue(TEXT("Byte: range"), HasError(TEXT("Byte"), TEXT("out of range")));
	TestTrue(TEXT("Enum: unknown name"), HasError(TEXT("Enum"), TEXT("is not a value of enum")));
	TestTrue(TEXT("Tag: unknown"), HasError(TEXT("Tag"), TEXT("Unknown gameplay tag")));
	TestTrue(TEXT("HardRef: missing"), HasError(TEXT("HardRef"), TEXT("not found")));
	TestTrue(TEXT("SingleChild: wrong $class"), HasError(TEXT("SingleChild.$class"), TEXT("is not a subclass")));
	TestTrue(TEXT("Children[0]: no $class"), HasError(TEXT("Children[0].$class"), TEXT("Missing required field")));
	TestTrue(TEXT("Speedd: unknown field"), HasError(TEXT("Speedd"), TEXT("Unknown field")));
	TestTrue(TEXT("ReadOnly: not editable"), HasError(TEXT("ReadOnly"), TEXT("not editable")));
	TestTrue(TEXT("IntToString[1]: duplicate"), HasError(TEXT("IntToString[1]"), TEXT("Duplicate key")));

	TestEqual(TEXT("No unexpected errors"), Ctx.GetErrorCount(), 10);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
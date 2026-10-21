#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DataBridgePipeline.h"
#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Interfaces/DataBridgeHandler.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
#include "Tests/DataBridgeTestTypes.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "DataBridgeSchemaBuilder.h"

namespace DataBridgeResolverTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	UDataBridgeTestAsset* MakeTransientCopy()
	{
		UPackage* Transient = GetTransientPackage();
		return NewObject<UDataBridgeTestAsset>(Transient, MakeUniqueObjectName(Transient, UDataBridgeTestAsset::StaticClass(), TEXT("Copy")));
	}

	UDataBridgeTestAsset* MakeTempAsset(const FString& PackageName)
	{
		UPackage* Package = CreatePackage(*PackageName);
		return NewObject<UDataBridgeTestAsset>(Package, *FPackageName::GetShortName(PackageName), RF_Transactional);
	}

	// 테스트 전용 핸들러: UDataBridgeTestAsset의 Value만 내보낸다
	class FTestAssetHandler : public IDataBridgeHandler
	{
	public:
		virtual UClass* GetSupportedClass() const override
		{
			return UDataBridgeTestAsset::StaticClass();
		}
		virtual TSharedPtr<FDataBridgeObject> Export(const UObject* Asset, const FDataBridgeReferenceResolver& Resolver, FDataBridgeContext& Ctx) const override
		{
			const TSharedRef<FDataBridgeObject> Data = MakeShared<FDataBridgeObject>();
			Data->SetNumberField(TEXT("Value"), CastChecked<UDataBridgeTestAsset>(Asset)->Value);
			return Data;
		}
		virtual bool Import(const FDataBridgeObject& Data, UObject* Target, const FDataBridgeReferenceResolver& Resolver, FDataBridgeContext& Ctx) const override
		{
			return false;
		}
		virtual void Describe(const UClass* Class, FDataBridgeSchemaBuilder& Schema, FDataBridgeContext& Ctx) const override
		{
		}
	};
}

// 주소록: 등록된 경로(두 표기), 외부 에셋, 역방향, 에러
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeResolverTest,
	"DataBridge.Resolver.MapsAndResolves", DataBridgeResolverTest::TestFlags)

	bool FDataBridgeResolverTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeResolverTest;

	FDataBridgeReferenceResolver Resolver;
	TStrongObjectPtr<UDataBridgeTestAsset> Copy(MakeTransientCopy());
	const FString Logical = TEXT("/Game/DataBridgeResolverTest/DA_Planned");
	Resolver.Register(Logical, Copy.Get());

	FDataBridgeContext Ctx;

	// 1. 등록된 경로 → 복제본 (짧은 표기, 오브젝트 표기 모두)
	TestTrue(TEXT("Registered: short form"), Resolver.ResolveHard(Logical, UDataBridgeTestAsset::StaticClass(), Ctx) == Copy.Get());
	TestTrue(TEXT("Registered: object form"), Resolver.ResolveHard(Logical + TEXT(".DA_Planned"), nullptr, Ctx) == Copy.Get());
	TestTrue(TEXT("Soft: planned target exists"), Resolver.CheckSoft(Logical, Ctx));

	// 2. 역방향
	TestEqual(TEXT("Reverse: registered copy -> logical path"), Resolver.ToLogicalPath(Copy.Get()), Logical);
	TestEqual(TEXT("Reverse: native class keeps full path"),
		Resolver.ToLogicalPath(UDataBridgeTestAsset::StaticClass()), FString(TEXT("/Script/DataBridge.DataBridgeTestAsset")));

	// 3. 문서 밖 외부 에셋 (엔진에 항상 있는 머티리얼, 읽기만)
	const FString EngineMaterial = TEXT("/Engine/EngineMaterials/DefaultMaterial");
	UObject* Material = Resolver.ResolveHard(EngineMaterial, UMaterialInterface::StaticClass(), Ctx);
	if (TestNotNull(TEXT("External asset resolved"), Material))
	{
		TestEqual(TEXT("Reverse: main asset uses short form"), Resolver.ToLogicalPath(Material), EngineMaterial);
	}
	TestTrue(TEXT("Soft: external target exists"), Resolver.CheckSoft(EngineMaterial, Ctx));
	TestFalse(TEXT("No errors in the valid cases"), Ctx.HasErrors());

	// 4. 에러 케이스
	FDataBridgeContext ErrorCtx;
	const FString Missing = TEXT("/Game/DataBridgeResolverTest/DA_Missing");
	TestNull(TEXT("Missing target"), Resolver.ResolveHard(Missing, nullptr, ErrorCtx));
	TestNull(TEXT("Wrong class"), Resolver.ResolveHard(Logical, UDataBridgeTestChild::StaticClass(), ErrorCtx));
	TestFalse(TEXT("Soft: missing target"), Resolver.CheckSoft(Missing, ErrorCtx));
	AddInfo(ErrorCtx.BuildReport());
	TestEqual(TEXT("Three errors reported"), ErrorCtx.GetErrorCount(), 3);

	return true;
}

// Export 봉투: 가장 가까운 핸들러 선택, 봉투 모양, 역방향 경로, 중복 에러
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeExportEnvelopeTest,
	"DataBridge.Export.Envelope", DataBridgeResolverTest::TestFlags)

	bool FDataBridgeExportEnvelopeTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeResolverTest;
	using namespace DataBridge;

	FTestAssetHandler Handler;
	FDataBridgeRegistry::Register(Handler);
	ON_SCOPE_EXIT{ FDataBridgeRegistry::Unregister(Handler); };

	// 창구: 거리 0인 테스트 핸들러가 UObject용 Reflection 핸들러를 이겨야 한다
	TestTrue(TEXT("Nearest handler wins"), FDataBridgeRegistry::FindHandler(UDataBridgeTestAsset::StaticClass()) == &Handler);
	TestTrue(TEXT("Other classes keep the reflection handler"), FDataBridgeRegistry::FindHandler(UDataBridgeTestChild::StaticClass()) != &Handler);

	const FString PackageName = FString::Printf(TEXT("/Temp/DataBridgeExportTest/Asset_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	TStrongObjectPtr<UDataBridgeTestAsset> Asset(MakeTempAsset(PackageName));
	Asset->Value = 42;

	// 1. 기본 Export
	{
		FDataBridgeContext Ctx;
		const TArray<UObject*> Assets = { Asset.Get() };
		const TSharedPtr<FDataBridgeObject> Root = FDataBridgePipeline::Export(Assets, Ctx);
		if (!TestTrue(TEXT("Exported"), Root.IsValid()))
		{
			AddError(Ctx.BuildReport());
			return false;
		}

		TestEqual(TEXT("format"), Root->GetStringField(Keys::Format), FString(FormatVersion));
		const TArray<TSharedPtr<FDataBridgeValue>>& Entries = Root->GetArrayField(Keys::Assets);
		if (TestEqual(TEXT("One asset"), Entries.Num(), 1))
		{
			const TSharedPtr<FDataBridgeObject>& Entry = Entries[0]->AsObject();
			TestEqual(TEXT("path = short package path"), Entry->GetStringField(Keys::Path), PackageName);
			TestEqual(TEXT("class = full path"), Entry->GetStringField(Keys::Class), FString(TEXT("/Script/DataBridge.DataBridgeTestAsset")));
			TestEqual(TEXT("data from the test handler"), Entry->GetObjectField(Keys::Data)->GetIntegerField(TEXT("Value")), 42);
		}

		// 실제 JSON 모양을 로그로 확인
		if (const IDataBridgeFormat* Json = FDataBridgeRegistry::FindFormatByExtension(TEXT("json")))
		{
			FString Text;
			Json->Write(Root.ToSharedRef(), Text, Ctx);
			AddInfo(Text);
		}
	}

	// 2. 주소록 지정: 임시 복제본이 원래 논리 경로로 나가는지 (Dry-run diff용)
	{
		TStrongObjectPtr<UDataBridgeTestAsset> Copy(MakeTransientCopy());
		FDataBridgeReferenceResolver Resolver;
		Resolver.Register(PackageName, Copy.Get());

		FDataBridgeContext Ctx;
		const TArray<UObject*> Assets = { Copy.Get() };
		const TSharedPtr<FDataBridgeObject> Root = FDataBridgePipeline::Export(Assets, Resolver, Ctx);
		if (TestTrue(TEXT("Copy exported"), Root.IsValid()))
		{
			TestEqual(TEXT("Copy uses the logical path"),
				Root->GetArrayField(Keys::Assets)[0]->AsObject()->GetStringField(Keys::Path), PackageName);
		}
		else
		{
			AddError(Ctx.BuildReport());
		}
	}

	// 3. 에러: 같은 에셋 두 번
	{
		FDataBridgeContext Ctx;
		const TArray<UObject*> Assets = { Asset.Get(), Asset.Get() };
		const TSharedPtr<FDataBridgeObject> Root = FDataBridgePipeline::Export(Assets, Ctx);
		AddInfo(Ctx.BuildReport());
		TestFalse(TEXT("Duplicate rejected"), Root.IsValid());
		TestTrue(TEXT("Error at assets[1]"), Ctx.GetIssues().ContainsByPredicate([](const FDataBridgeIssue& Issue)
			{
				return Issue.Path == TEXT("assets[1]") && Issue.Message.Contains(TEXT("more than once"));
			}));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
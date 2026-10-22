#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DataBridgePipeline.h"
#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeSchemaBuilder.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Handlers/DataBridgeExportWalker.h"
#include "Handlers/DataBridgeImportWalker.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Tests/DataBridgeTestFixtures.h"
#include "Tests/DataBridgeTestTypes.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace DataBridgeDescribeTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	TSharedPtr<FDataBridgeObject> GetObject(const TSharedPtr<FDataBridgeObject>& Parent, const FString& Field)
	{
		const TSharedPtr<FDataBridgeObject>* Out = nullptr;
		return (Parent.IsValid() && Parent->TryGetObjectField(Field, Out)) ? *Out : nullptr;
	}

	// classes / structs 테이블의 정의
	TSharedPtr<FDataBridgeObject> GetDefinition(const TSharedPtr<FDataBridgeObject>& Doc, const TCHAR* Table, const UStruct* Struct)
	{
		return GetObject(GetObject(Doc, Table), Struct->GetPathName());
	}

	TSharedPtr<FDataBridgeObject> GetField(const TSharedPtr<FDataBridgeObject>& Definition, const FString& Name)
	{
		return GetObject(GetObject(Definition, DataBridge::SchemaKeys::Fields), Name);
	}

	FString GetString(const TSharedPtr<FDataBridgeObject>& Object, const TCHAR* Field)
	{
		FString Out;
		if (Object.IsValid())
		{
			Object->TryGetStringField(Field, Out);
		}
		return Out;
	}

	// ["A", "B"] → "A,B"
	FString JoinStrings(const TSharedPtr<FDataBridgeObject>& Object, const TCHAR* Field)
	{
		const TArray<TSharedPtr<FDataBridgeValue>>* Items = nullptr;
		if (!Object.IsValid() || !Object->TryGetArrayField(Field, Items))
		{
			return TEXT("(none)");
		}
		TArray<FString> Names;
		for (const TSharedPtr<FDataBridgeValue>& Item : *Items)
		{
			Names.Add(Item->AsString());
		}
		return FString::Join(Names, TEXT(","));
	}

	bool ArrayContains(const TSharedPtr<FDataBridgeObject>& Object, const FString& Field, const FString& Value)
	{
		const TArray<TSharedPtr<FDataBridgeValue>>* Items = nullptr;
		return Object.IsValid() && Object->TryGetArrayField(Field, Items)
			&& Items->ContainsByPredicate([&Value](const TSharedPtr<FDataBridgeValue>& Item)
				{
					return Item.IsValid() && Item->Type == EJson::String && Item->AsString() == Value;
				});
	}

	TSet<FString> KeysOf(const TSharedPtr<FDataBridgeObject>& Object)
	{
		TSet<FString> Keys;
		if (Object.IsValid())
		{
			for (const TPair<FString, TSharedPtr<FDataBridgeValue>>& Pair : Object->Values)
			{
				Keys.Add(Pair.Key);
			}
		}
		return Keys;
	}

	TSharedPtr<FDataBridgeObject> DescribeAllTypes(FDataBridgeContext& Ctx, const TArray<FString>& Roots = TArray<FString>())
	{
		return FDataBridgePipeline::Describe(UDataBridgeTestAllTypes::StaticClass(), Ctx, Roots);
	}

	bool ImportJson(const FString& Text, UObject* Target, FDataBridgeContext& Ctx)
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

	bool HasError(const FDataBridgeContext& Ctx, const TCHAR* Path, const TCHAR* MessagePart)
	{
		return Ctx.GetIssues().ContainsByPredicate([&](const FDataBridgeIssue& Issue)
			{
				return Issue.Severity == EDataBridgeSeverity::Error && Issue.Path == Path && Issue.Message.Contains(MessagePart);
			});
	}
}

// 1. 타입별 설명이 Walker 규칙과 같은가
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeDescribeAllTypesTest,
	"DataBridge.Describe.DescribesAllTypes", DataBridgeDescribeTest::TestFlags)

	bool FDataBridgeDescribeAllTypesTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeDescribeTest;
	using namespace DataBridge;

	FDataBridgeContext Ctx;
	const TSharedPtr<FDataBridgeObject> Doc = DescribeAllTypes(Ctx);
	if (!TestTrue(TEXT("Described"), Doc.IsValid()))
	{
		AddError(Ctx.BuildReport());
		return false;
	}
	if (Ctx.GetIssues().Num() > 0)
	{
		AddInfo(Ctx.BuildReport());
	}

	const TSharedPtr<FDataBridgeObject> Def = GetDefinition(Doc, SchemaKeys::Classes, UDataBridgeTestAllTypes::StaticClass());
	if (!TestTrue(TEXT("AllTypes is in classes"), Def.IsValid()))
	{
		return false;
	}

	const auto TypeOf = [&Def](const TCHAR* Name) { return GetString(GetField(Def, Name), SchemaKeys::Type); };
	const auto ItemTypeOf = [&Def](const TCHAR* Name) { return GetString(GetObject(GetField(Def, Name), SchemaKeys::Items), SchemaKeys::Type); };

	// ---- 기본 타입 ----
	TestEqual(TEXT("bool"), TypeOf(TEXT("bFlag")), FString(SchemaTypes::Bool));
	TestEqual(TEXT("int32"), TypeOf(TEXT("Int")), FString(SchemaTypes::Int));
	TestEqual(TEXT("int64"), TypeOf(TEXT("BigInt")), FString(SchemaTypes::Int));
	TestEqual(TEXT("uint8"), TypeOf(TEXT("Byte")), FString(SchemaTypes::Int));
	TestEqual(TEXT("float"), TypeOf(TEXT("Float")), FString(SchemaTypes::Float));
	TestEqual(TEXT("double"), TypeOf(TEXT("Double")), FString(SchemaTypes::Float));
	TestEqual(TEXT("FString"), TypeOf(TEXT("Str")), FString(SchemaTypes::String));
	TestEqual(TEXT("FName"), TypeOf(TEXT("Name")), FString(SchemaTypes::Name));
	TestEqual(TEXT("FText"), TypeOf(TEXT("Text")), FString(SchemaTypes::Text));

	// ---- 값 목록 ----
	TestEqual(TEXT("enum type"), TypeOf(TEXT("Enum")), FString(SchemaTypes::Enum));
	TestEqual(TEXT("enum values (no _MAX)"), JoinStrings(GetField(Def, TEXT("Enum")), SchemaKeys::Values), FString(TEXT("Alpha,Beta,Gamma")));
	TestEqual(TEXT("flags type"), TypeOf(TEXT("MaskFlags")), FString(SchemaTypes::Flags));
	TestEqual(TEXT("flags values (value = bit, Hidden skipped)"), JoinStrings(GetField(Def, TEXT("MaskFlags")), SchemaKeys::Values), FString(TEXT("Red,Green,Blue")));
	TestEqual(TEXT("flags values (value = bit index)"), JoinStrings(GetField(Def, TEXT("BitIndexFlags")), SchemaKeys::Values), FString(TEXT("First,Second,Third")));

	// ---- 구조체: 정의 테이블에 한 번, 필드는 이름으로 가리킨다 ----
	const FString StructPath = FDataBridgeTestStruct::StaticStruct()->GetPathName();
	TestEqual(TEXT("struct type"), TypeOf(TEXT("Struct")), FString(SchemaTypes::Struct));
	TestEqual(TEXT("struct refers by path"), GetString(GetField(Def, TEXT("Struct")), SchemaKeys::Struct), StructPath);
	const TSharedPtr<FDataBridgeObject> StructDef = GetDefinition(Doc, SchemaKeys::Structs, FDataBridgeTestStruct::StaticStruct());
	TestEqual(TEXT("struct definition: Count"), GetString(GetField(StructDef, TEXT("Count")), SchemaKeys::Type), FString(SchemaTypes::Int));
	TestEqual(TEXT("struct definition: Label"), GetString(GetField(StructDef, TEXT("Label")), SchemaKeys::Type), FString(SchemaTypes::String));
	TestTrue(TEXT("engine struct (FVector) is defined too"), GetDefinition(Doc, SchemaKeys::Structs, TBaseStructure<FVector>::Get()).IsValid());

	// ---- 컨테이너 ----
	TestEqual(TEXT("TArray<int32>"), TypeOf(TEXT("IntArray")), FString(SchemaTypes::Array));
	TestEqual(TEXT("TArray<int32> items"), ItemTypeOf(TEXT("IntArray")), FString(SchemaTypes::Int));
	TestEqual(TEXT("TArray<struct> items"), ItemTypeOf(TEXT("StructArray")), FString(SchemaTypes::Struct));
	TestEqual(TEXT("TSet<FName>"), TypeOf(TEXT("NameSet")), FString(SchemaTypes::Set));
	TestEqual(TEXT("TSet<FName> items"), ItemTypeOf(TEXT("NameSet")), FString(SchemaTypes::Name));
	TestEqual(TEXT("TMap<FName,int32> form"), GetString(GetField(Def, TEXT("NameToInt")), SchemaKeys::MapForm), FString(TEXT("object")));
	TestEqual(TEXT("TMap<Tag,int32> form"), GetString(GetField(Def, TEXT("TagToInt")), SchemaKeys::MapForm), FString(TEXT("object")));
	TestEqual(TEXT("TMap<Tag,int32> key"), GetString(GetObject(GetField(Def, TEXT("TagToInt")), SchemaKeys::Key), SchemaKeys::Type), FString(SchemaTypes::Tag));
	TestEqual(TEXT("TMap<int32,FString> form"), GetString(GetField(Def, TEXT("IntToString")), SchemaKeys::MapForm), FString(TEXT("pairs")));

	// ---- 참조 ----
	TestEqual(TEXT("hard ref"), TypeOf(TEXT("HardRef")), FString(SchemaTypes::Asset));
	TestEqual(TEXT("soft ref"), TypeOf(TEXT("SoftRef")), FString(SchemaTypes::Asset));
	TestEqual(TEXT("class ref"), TypeOf(TEXT("ClassRef")), FString(SchemaTypes::Class));
	TestEqual(TEXT("soft class ref"), TypeOf(TEXT("SoftClassRef")), FString(SchemaTypes::Class));
	TestEqual(TEXT("class ref base"), GetString(GetField(Def, TEXT("ChildClassRef")), SchemaKeys::Class), UDataBridgeTestChild::StaticClass()->GetPathName());
	TestEqual(TEXT("instanced"), TypeOf(TEXT("SingleChild")), FString(SchemaTypes::Instanced));
	TestEqual(TEXT("instanced array items"), ItemTypeOf(TEXT("Children")), FString(SchemaTypes::Instanced));

	// ---- 포매터가 말한 모양 ----
	TestEqual(TEXT("FGameplayTag"), TypeOf(TEXT("Tag")), FString(SchemaTypes::Tag));
	TestEqual(TEXT("FGameplayTagContainer"), TypeOf(TEXT("Tags")), FString(SchemaTypes::Tags));
	TestEqual(TEXT("FTransform"), TypeOf(TEXT("Transform")), FString(SchemaTypes::Transform));

	// ---- Walker가 다루지 않는 필드는 설명서에도 없다 ----
	TestFalse(TEXT("Transient omitted"), GetField(Def, TEXT("TransientValue")).IsValid());
	TestFalse(TEXT("Not editable omitted"), GetField(Def, TEXT("NotEditable")).IsValid());
	TestFalse(TEXT("VisibleAnywhere omitted"), GetField(Def, TEXT("ReadOnly")).IsValid());

	return true;
}

// 2. 계약: 설명서의 필드 = Export가 내보내는 키 (양방향). 한쪽만 고치면 여기서 잡힌다
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeDescribeContractTest,
	"DataBridge.Describe.MatchesExportFields", DataBridgeDescribeTest::TestFlags)

	bool FDataBridgeDescribeContractTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeDescribeTest;
	using namespace DataBridge;

	FDataBridgeContext Ctx;
	const TSharedPtr<FDataBridgeObject> Doc = DescribeAllTypes(Ctx);
	if (!TestTrue(TEXT("Described"), Doc.IsValid()))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	const auto Compare = [this](const FString& What, const TSet<FString>& Described, const TSet<FString>& Exported)
		{
			TestTrue(FString::Printf(TEXT("%s has fields"), *What), Described.Num() > 0 && Exported.Num() > 0);
			for (const FString& Key : Exported)
			{
				if (!Described.Contains(Key))
				{
					AddError(FString::Printf(TEXT("%s: '%s' is exported but not described"), *What, *Key));
				}
			}
			for (const FString& Key : Described)
			{
				if (!Exported.Contains(Key))
				{
					AddError(FString::Printf(TEXT("%s: '%s' is described but not exported"), *What, *Key));
				}
			}
		};

	const FDataBridgeReferenceResolver Resolver;
	FDataBridgeContext ExportCtx;

	// 루트 클래스
	TStrongObjectPtr<UDataBridgeTestAllTypes> Filled(DataBridgeTestFixtures::MakeFilledAllTypes());
	const TSharedPtr<FDataBridgeObject> Exported = FDataBridgeExportWalker(Resolver, ExportCtx).ExportObject(Filled.Get());
	const TSharedPtr<FDataBridgeObject> Def = GetDefinition(Doc, SchemaKeys::Classes, UDataBridgeTestAllTypes::StaticClass());
	Compare(TEXT("AllTypes"), KeysOf(GetObject(Def, SchemaKeys::Fields)), KeysOf(Exported));

	// 구조체 (정의 테이블 vs 실제 값)
	const TSharedPtr<FDataBridgeObject> StructDef = GetDefinition(Doc, SchemaKeys::Structs, FDataBridgeTestStruct::StaticStruct());
	Compare(TEXT("DataBridgeTestStruct"), KeysOf(GetObject(StructDef, SchemaKeys::Fields)), KeysOf(GetObject(Exported, TEXT("Struct"))));

	// Instanced로 고를 수 있는 클래스
	TStrongObjectPtr<UDataBridgeTestPickableChild> Child(NewObject<UDataBridgeTestPickableChild>(GetTransientPackage()));
	const TSharedPtr<FDataBridgeObject> ChildExported = FDataBridgeExportWalker(Resolver, ExportCtx).ExportObject(Child.Get());
	const TSharedPtr<FDataBridgeObject> ChildDef = GetDefinition(Doc, SchemaKeys::Classes, UDataBridgeTestPickableChild::StaticClass());
	Compare(TEXT("PickableChild"), KeysOf(GetObject(ChildDef, SchemaKeys::Fields)), KeysOf(ChildExported));

	return true;
}

// 3. 주석, 범위, allowed 필터, 기본값 + Import의 Clamp 검사
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeDescribeRulesTest,
	"DataBridge.Describe.DocumentationAndRules", DataBridgeDescribeTest::TestFlags)

	bool FDataBridgeDescribeRulesTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeDescribeTest;
	using namespace DataBridge;

	FDataBridgeContext Ctx;
	const TSharedPtr<FDataBridgeObject> Doc = DescribeAllTypes(Ctx);
	if (!TestTrue(TEXT("Described"), Doc.IsValid()))
	{
		AddError(Ctx.BuildReport());
		return false;
	}
	const TSharedPtr<FDataBridgeObject> Def = GetDefinition(Doc, SchemaKeys::Classes, UDataBridgeTestAllTypes::StaticClass());

	// ---- 주석 → description (블록 주석의 '*' 정리) ----
	const TSharedPtr<FDataBridgeObject> Clamped = GetField(Def, TEXT("ClampedInt"));
	const FString Description = GetString(Clamped, SchemaKeys::Description);
	AddInfo(FString::Printf(TEXT("ClampedInt description: [%s]"), *Description));
	TestTrue(TEXT("Block comment becomes description"), Description.Contains(TEXT("Documented test field.")) && Description.Contains(TEXT("Second line.")));
	TestFalse(TEXT("Leading '*' removed"), Description.Contains(TEXT("*")));

	// ---- ClampMin/ClampMax → min/max ----
	double Min = -1.0, Max = -1.0;
	TestTrue(TEXT("min/max present"), Clamped.IsValid() && Clamped->TryGetNumberField(SchemaKeys::Min, Min) && Clamped->TryGetNumberField(SchemaKeys::Max, Max));
	TestEqual(TEXT("min"), Min, 0.0);
	TestEqual(TEXT("max"), Max, 10.0);

	// ---- undocumented: 주석 없는 우리 필드만, 엔진 타입은 제외 ----
	TestTrue(TEXT("Field without comment is listed"), ArrayContains(Doc, SchemaKeys::Undocumented, TEXT("DataBridgeTestAllTypes.ChildClassRef")));
	TestFalse(TEXT("Documented field is not listed"), ArrayContains(Doc, SchemaKeys::Undocumented, TEXT("DataBridgeTestAllTypes.ClampedInt")));
	TestFalse(TEXT("Engine struct fields are not listed"), ArrayContains(Doc, SchemaKeys::Undocumented, TEXT("Vector.X")));

	// ---- allowed: 에디터 드롭다운 규칙 ----
	const TSharedPtr<FDataBridgeObject> ChildrenItems = GetObject(GetField(Def, TEXT("Children")), SchemaKeys::Items);
	TestTrue(TEXT("Pickable class is allowed"), ArrayContains(ChildrenItems, SchemaKeys::Allowed, UDataBridgeTestPickableChild::StaticClass()->GetPathName()));
	TestFalse(TEXT("HideDropDown class is not allowed"), ArrayContains(ChildrenItems, SchemaKeys::Allowed, UDataBridgeTestChild::StaticClass()->GetPathName()));
	TestTrue(TEXT("Allowed class is described"), GetDefinition(Doc, SchemaKeys::Classes, UDataBridgeTestPickableChild::StaticClass()).IsValid());

	// ---- defaults ----
	double DefaultClamped = -1.0;
	const TSharedPtr<FDataBridgeObject> Defaults = GetObject(Def, SchemaKeys::Defaults);
	TestTrue(TEXT("Defaults present"), Defaults.IsValid() && Defaults->TryGetNumberField(TEXT("ClampedInt"), DefaultClamped) && DefaultClamped == 0.0);
	TestTrue(TEXT("Allowed class has defaults"), GetObject(GetDefinition(Doc, SchemaKeys::Classes, UDataBridgeTestPickableChild::StaticClass()), SchemaKeys::Defaults).IsValid());

	// ---- Import: Clamp 위반은 잘라주지 않고 에러 ----
	TStrongObjectPtr<UDataBridgeTestAllTypes> Obj(DataBridgeTestFixtures::MakeEmptyAllTypes(TEXT("DescribeClamp")));
	{
		FDataBridgeContext ImportCtx;
		TestFalse(TEXT("Above ClampMax rejected"), ImportJson(TEXT(R"({ "ClampedInt": 11 })"), Obj.Get(), ImportCtx));
		TestTrue(TEXT("Above ClampMax error"), HasError(ImportCtx, TEXT("ClampedInt"), TEXT("above ClampMax")));
	}
	{
		FDataBridgeContext ImportCtx;
		TestFalse(TEXT("Below ClampMin rejected"), ImportJson(TEXT(R"({ "ClampedInt": -1 })"), Obj.Get(), ImportCtx));
		TestTrue(TEXT("Below ClampMin error"), HasError(ImportCtx, TEXT("ClampedInt"), TEXT("below ClampMin")));
	}
	TestEqual(TEXT("Rejected values were not written"), Obj->ClampedInt, 0);
	{
		FDataBridgeContext ImportCtx;
		TestTrue(TEXT("Boundary value accepted"), ImportJson(TEXT(R"({ "ClampedInt": 10 })"), Obj.Get(), ImportCtx));
		TestEqual(TEXT("Boundary value written"), Obj->ClampedInt, 10);
	}

	return true;
}

// 4. 카탈로그: 태그, 클래스 후보(C++ 자식 포함), 범위 인자
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeDescribeCatalogTest,
	"DataBridge.Describe.Catalog", DataBridgeDescribeTest::TestFlags)

	bool FDataBridgeDescribeCatalogTest::RunTest(const FString& Parameters)
{
	using namespace DataBridgeDescribeTest;
	using namespace DataBridge;

	const FString ChildPath = UDataBridgeTestChild::StaticClass()->GetPathName();
	const FString PickablePath = UDataBridgeTestPickableChild::StaticClass()->GetPathName();
	const FString ObjectPath = UObject::StaticClass()->GetPathName();

	// 범위 지정 (끝의 '/'는 정리된다)
	{
		FDataBridgeContext Ctx;
		const TSharedPtr<FDataBridgeObject> Doc = DescribeAllTypes(Ctx, { TEXT("/Temp/DataBridgeDescribeTest/") });
		if (!TestTrue(TEXT("Described"), Doc.IsValid()))
		{
			AddError(Ctx.BuildReport());
			return false;
		}
		const TSharedPtr<FDataBridgeObject> Catalog = GetObject(Doc, SchemaKeys::Catalog);
		TestEqual(TEXT("Roots recorded and trimmed"), JoinStrings(Catalog, SchemaKeys::Roots), FString(TEXT("/Temp/DataBridgeDescribeTest")));

		const TSharedPtr<FDataBridgeObject> Tags = GetObject(Catalog, SchemaKeys::Tags);
		TestTrue(TEXT("Registered tag listed"), Tags.IsValid() && Tags->HasField(TEXT("DataBridge.Test.A")));

		const TSharedPtr<FDataBridgeObject> Classes = GetObject(Catalog, SchemaKeys::Classes);
		TestTrue(TEXT("C++ subclass listed for a class field (GetDerivedClassNames returns native classes)"), ArrayContains(Classes, ChildPath, PickablePath));
		TestFalse(TEXT("UObject class field skipped"), Classes.IsValid() && Classes->HasField(ObjectPath));

		const TSharedPtr<FDataBridgeObject> Assets = GetObject(Catalog, SchemaKeys::Assets);
		TestFalse(TEXT("UObject asset field skipped"), Assets.IsValid() && Assets->HasField(ObjectPath));
	}

	// 범위 생략 → /Game
	{
		FDataBridgeContext Ctx;
		const TSharedPtr<FDataBridgeObject> Doc = DescribeAllTypes(Ctx);
		TestEqual(TEXT("Default root is /Game"), JoinStrings(GetObject(Doc, SchemaKeys::Catalog), SchemaKeys::Roots), FString(TEXT("/Game")));
	}

	// 잘못된 범위 → 실패
	{
		FDataBridgeContext Ctx;
		const TSharedPtr<FDataBridgeObject> Doc = DescribeAllTypes(Ctx, { TEXT("Game/Oops") });
		TestFalse(TEXT("Invalid root rejected"), Doc.IsValid());
		TestTrue(TEXT("Invalid root error"), Ctx.GetIssues().ContainsByPredicate([](const FDataBridgeIssue& Issue)
			{
				return Issue.Severity == EDataBridgeSeverity::Error && Issue.Message.Contains(TEXT("must be a content path"));
			}));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
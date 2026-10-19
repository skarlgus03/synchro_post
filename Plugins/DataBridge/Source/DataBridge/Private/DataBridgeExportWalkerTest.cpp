#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "DataBridgeTypes.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Handlers/DataBridgeExportWalker.h"
#include "Interfaces/DataBridgeFormat.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "NativeGameplayTags.h"
#include "Tests/DataBridgeTestTypes.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"


// 테스트 전용 태그 (프로젝트 태그에 의존하지 않기 위해)
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DataBridgeTest_A, "DataBridge.Test.A");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DataBridgeTest_B, "DataBridge.Test.B");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDataBridgeExportWalkerTest,
	"DataBridge.Walker.ExportAllTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDataBridgeExportWalkerTest::RunTest(const FString& Parameters)
{
	const FString PackageName = FString::Printf(TEXT("/Temp/DataBridgeWalkerTest/AllTypes_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	UPackage* Package = CreatePackage(*PackageName);
	TStrongObjectPtr<UDataBridgeTestAllTypes> Obj(NewObject<UDataBridgeTestAllTypes>(Package, *FPackageName::GetShortName(PackageName), RF_Transactional));

	// ---- 값 채우기 ----
	Obj->bFlag = true;
	Obj->Int = -7;
	Obj->BigInt = 1234567890123;
	Obj->Float = 1.5f;
	Obj->Double = 2.25;
	Obj->Byte = 200;
	Obj->Str = TEXT("hello");
	Obj->Name = TEXT("SomeName");
	Obj->Text = FText::FromString(TEXT("Fire Skeleton"));
	Obj->Enum = EDataBridgeTestEnum::Gamma;
	Obj->Vector = FVector(1, 2, 3);
	Obj->Rotator = FRotator(10, 20, 30);
	Obj->Struct.Count = 3;
	Obj->Struct.Label = TEXT("three");
	Obj->Tag = TAG_DataBridgeTest_A;
	Obj->Tags.AddTag(TAG_DataBridgeTest_A);
	Obj->Tags.AddTag(TAG_DataBridgeTest_B);
	Obj->IntArray = { 1, 2, 3 };
	Obj->StructArray.AddDefaulted_GetRef().Label = TEXT("one");
	Obj->NameSet.Add(TEXT("X"));
	Obj->NameToInt.Add(TEXT("Key"), 5);
	Obj->TagToInt.Add(TAG_DataBridgeTest_B, 9);
	Obj->IntToString.Add(4, TEXT("four"));

	const FString MaterialPath = TEXT("/Engine/EngineMaterials/DefaultMaterial");
	Obj->HardRef = LoadObject<UMaterialInterface>(nullptr, *(MaterialPath + TEXT(".DefaultMaterial")));
	Obj->SoftRef = TSoftObjectPtr<UObject>(FSoftObjectPath(MaterialPath + TEXT(".DefaultMaterial")));
	Obj->ClassRef = UDataBridgeTestChild::StaticClass();
	Obj->SoftClassRef = TSoftClassPtr<UObject>(UDataBridgeTestAsset::StaticClass());

	Obj->SingleChild = NewObject<UDataBridgeTestChild>(Obj.Get(), NAME_None, RF_Transactional);
	Obj->SingleChild->Value = 11;
	for (int32 Value : { 21, 22 })
	{
		UDataBridgeTestChild* Child = NewObject<UDataBridgeTestChild>(Obj.Get(), NAME_None, RF_Transactional);
		Child->Value = Value;
		Obj->Children.Add(Child);
	}

	Obj->TransientValue = 99;
	Obj->NotEditable = 99;
	Obj->ReadOnly = 99;

	// ---- Export ----
	FDataBridgeContext Ctx;
	const FDataBridgeReferenceResolver Resolver;
	FDataBridgeExportWalker Walker(Resolver, Ctx);
	const TSharedPtr<FDataBridgeObject> D = Walker.ExportObject(Obj.Get());
	if (!TestTrue(TEXT("Exported"), D.IsValid()))
	{
		AddError(Ctx.BuildReport());
		return false;
	}

	if (const IDataBridgeFormat* Json = FDataBridgeRegistry::FindFormatByExtension(TEXT("json")))
	{
		FString Text;
		Json->Write(D.ToSharedRef(), Text, Ctx);
		AddInfo(Text);
	}
	if (Ctx.GetIssues().Num() > 0)
	{
		AddInfo(Ctx.BuildReport());
	}

	// ---- 기본 타입 ----
	TestTrue(TEXT("bool"), D->GetBoolField(TEXT("bFlag")));
	TestEqual(TEXT("int32"), D->GetIntegerField(TEXT("Int")), -7);
	TestEqual(TEXT("int64"), D->GetNumberField(TEXT("BigInt")), 1234567890123.0);
	TestEqual(TEXT("float"), D->GetNumberField(TEXT("Float")), 1.5);
	TestEqual(TEXT("double"), D->GetNumberField(TEXT("Double")), 2.25);
	TestEqual(TEXT("uint8"), D->GetIntegerField(TEXT("Byte")), 200);
	TestEqual(TEXT("FString"), D->GetStringField(TEXT("Str")), FString(TEXT("hello")));
	TestEqual(TEXT("FName"), D->GetStringField(TEXT("Name")), FString(TEXT("SomeName")));
	TestEqual(TEXT("FText"), D->GetStringField(TEXT("Text")), FString(TEXT("Fire Skeleton")));
	TestEqual(TEXT("enum by name"), D->GetStringField(TEXT("Enum")), FString(TEXT("Gamma")));

	// ---- struct ----
	TestEqual(TEXT("FVector.Y"), D->GetObjectField(TEXT("Vector"))->GetNumberField(TEXT("Y")), 2.0);
	TestEqual(TEXT("FRotator.Yaw"), D->GetObjectField(TEXT("Rotator"))->GetNumberField(TEXT("Yaw")), 20.0);
	TestEqual(TEXT("custom struct"), D->GetObjectField(TEXT("Struct"))->GetStringField(TEXT("Label")), FString(TEXT("three")));

	// ---- 포매터 ----
	TestEqual(TEXT("GameplayTag"), D->GetStringField(TEXT("Tag")), FString(TEXT("DataBridge.Test.A")));
	{
		const TArray<TSharedPtr<FDataBridgeValue>>& Tags = D->GetArrayField(TEXT("Tags"));
		if (TestEqual(TEXT("TagContainer count"), Tags.Num(), 2))
		{
			TestEqual(TEXT("TagContainer[1]"), Tags[1]->AsString(), FString(TEXT("DataBridge.Test.B")));
		}
	}

	// ---- 컨테이너 ----
	TestEqual(TEXT("TArray<int32>"), D->GetArrayField(TEXT("IntArray")).Num(), 3);
	TestEqual(TEXT("TArray<struct>"), D->GetArrayField(TEXT("StructArray"))[0]->AsObject()->GetStringField(TEXT("Label")), FString(TEXT("one")));
	TestEqual(TEXT("TSet<FName>"), D->GetArrayField(TEXT("NameSet"))[0]->AsString(), FString(TEXT("X")));
	TestEqual(TEXT("TMap<FName,int32> as object"), D->GetObjectField(TEXT("NameToInt"))->GetIntegerField(TEXT("Key")), 5);
	TestEqual(TEXT("TMap<Tag,int32> as object"), D->GetObjectField(TEXT("TagToInt"))->GetIntegerField(TEXT("DataBridge.Test.B")), 9);
	{
		const TArray<TSharedPtr<FDataBridgeValue>>& Pairs = D->GetArrayField(TEXT("IntToString"));
		if (TestEqual(TEXT("TMap<int32,FString> as pairs"), Pairs.Num(), 1))
		{
			TestEqual(TEXT("pair key"), Pairs[0]->AsObject()->GetIntegerField(TEXT("key")), 4);
			TestEqual(TEXT("pair value"), Pairs[0]->AsObject()->GetStringField(TEXT("value")), FString(TEXT("four")));
		}
	}

	// ---- 참조 ----
	TestEqual(TEXT("hard ref -> short path"), D->GetStringField(TEXT("HardRef")), MaterialPath);
	TestEqual(TEXT("soft ref -> short path"), D->GetStringField(TEXT("SoftRef")), MaterialPath);
	TestEqual(TEXT("class ref -> full path"), D->GetStringField(TEXT("ClassRef")), FString(TEXT("/Script/DataBridge.DataBridgeTestChild")));
	TestEqual(TEXT("soft class ref"), D->GetStringField(TEXT("SoftClassRef")), FString(TEXT("/Script/DataBridge.DataBridgeTestAsset")));

	// ---- Instanced ----
	{
		const TSharedPtr<FDataBridgeObject> Single = D->GetObjectField(TEXT("SingleChild"));
		TestEqual(TEXT("instanced $class"), Single->GetStringField(TEXT("$class")), FString(TEXT("/Script/DataBridge.DataBridgeTestChild")));
		TestEqual(TEXT("instanced field"), Single->GetIntegerField(TEXT("Value")), 11);

		const TArray<TSharedPtr<FDataBridgeValue>>& Children = D->GetArrayField(TEXT("Children"));
		if (TestEqual(TEXT("instanced array count"), Children.Num(), 2))
		{
			TestEqual(TEXT("instanced array [1]"), Children[1]->AsObject()->GetIntegerField(TEXT("Value")), 22);
		}
	}

	// ---- 제외 대상 ----
	TestFalse(TEXT("Transient excluded"), D->HasField(TEXT("TransientValue")));
	TestFalse(TEXT("Non-editable excluded"), D->HasField(TEXT("NotEditable")));
	TestFalse(TEXT("VisibleAnywhere excluded"), D->HasField(TEXT("ReadOnly")));

	TestEqual(TEXT("No issues"), Ctx.GetIssues().Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
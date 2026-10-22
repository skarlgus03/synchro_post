#include "Tests/DataBridgeTestFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Tests/DataBridgeTestTypes.h"
#include "UObject/Package.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_DataBridgeTest_A, "DataBridge.Test.A");
UE_DEFINE_GAMEPLAY_TAG(TAG_DataBridgeTest_B, "DataBridge.Test.B");

namespace DataBridgeTestFixtures
{
	FString UniqueTempPackageName(const TCHAR* Folder, const TCHAR* Prefix)
	{
		return FString::Printf(TEXT("/Temp/%s/%s_%s"), Folder, Prefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	UDataBridgeTestAllTypes* MakeEmptyAllTypes(const TCHAR* Prefix)
	{
		const FString PackageName = UniqueTempPackageName(TEXT("DataBridgeTest"), Prefix);
		UPackage* Package = CreatePackage(*PackageName);
		return NewObject<UDataBridgeTestAllTypes>(Package, *FPackageName::GetShortName(PackageName), RF_Transactional);
	}

	UDataBridgeTestAllTypes* MakeFilledAllTypes()
	{
		UDataBridgeTestAllTypes* Obj = MakeEmptyAllTypes(TEXT("Filled"));

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
		Obj->MaskFlags = 1 | 8;            // Red | Blue
		Obj->BitIndexFlags = (1 << 0) | (1 << 2);   // First | Third

		Obj->ClampedInt = 3;
		Obj->ChildClassRef = UDataBridgeTestPickableChild::StaticClass();

		Obj->Vector = FVector(1, 2, 3);
		Obj->Rotator = FRotator(10, 20, 30);
		Obj->Transform = FTransform(FRotator(0, 90, 0), FVector(10, 0, 5), FVector(1, 1, 2));
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

		const TCHAR* MaterialObjectPath = TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial");
		Obj->HardRef = LoadObject<UMaterialInterface>(nullptr, MaterialObjectPath);
		Obj->SoftRef = TSoftObjectPtr<UObject>(FSoftObjectPath(MaterialObjectPath));
		Obj->ClassRef = UDataBridgeTestChild::StaticClass();
		Obj->SoftClassRef = TSoftClassPtr<UObject>(UDataBridgeTestAsset::StaticClass());

		Obj->SingleChild = NewObject<UDataBridgeTestChild>(Obj, NAME_None, RF_Transactional);
		Obj->SingleChild->Value = 11;
		for (int32 Value : { 21, 22 })
		{
			UDataBridgeTestChild* Child = NewObject<UDataBridgeTestChild>(Obj, NAME_None, RF_Transactional);
			Child->Value = Value;
			Obj->Children.Add(Child);
		}
		return Obj;
	}
}

#endif
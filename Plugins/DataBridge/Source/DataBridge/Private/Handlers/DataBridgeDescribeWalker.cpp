#include "Handlers/DataBridgeDescribeWalker.h"

#include "DataBridgeRegistry.h"
#include "DataBridgeSchemaBuilder.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Handlers/DataBridgeBitmask.h"
#include "Handlers/DataBridgeExportWalker.h"
#include "Interfaces/DataBridgeValueFormatter.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace DataBridgeDescribeWalkerPrivate
{
	TSharedRef<FDataBridgeObject> MakeType(const TCHAR* Type)
	{
		TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
		Out->SetStringField(DataBridge::SchemaKeys::Type, Type);
		return Out;
	}

	void SetStringArray(FDataBridgeObject& Out, const TCHAR* Field, const TArray<FString>& Values)
	{
		TArray<TSharedPtr<FDataBridgeValue>> Items;
		for (const FString& Value : Values)
		{
			Items.Add(MakeShared<FJsonValueString>(Value));
		}
		Out.SetArrayField(Field, Items);
	}

	// 주석(UHT가 만든 ToolTip). 없으면 빈 문자열. FProperty와 UStruct/UClass 둘 다 받는다
	template <typename TField>
	FString GetToolTip(const TField* Field)
	{
#if WITH_EDITORONLY_DATA
		if (Field->HasMetaData(TEXT("ToolTip")))
		{
			return Field->GetMetaData(TEXT("ToolTip")).TrimStartAndEnd();
		}
#endif
		return FString();
	}

	// ClampMin/ClampMax → min/max
	void SetRange(FDataBridgeObject& Out, const FProperty* Property)
	{
#if WITH_EDITORONLY_DATA
		const TPair<const TCHAR*, const TCHAR*> Pairs[] = {
			{ TEXT("ClampMin"), DataBridge::SchemaKeys::Min },
			{ TEXT("ClampMax"), DataBridge::SchemaKeys::Max },
		};
		for (const TPair<const TCHAR*, const TCHAR*>& Pair : Pairs)
		{
			double Number = 0.0;
			if (Property->HasMetaData(Pair.Key) && LexTryParseString(Number, *Property->GetMetaData(Pair.Key)))
			{
				Out.SetNumberField(Pair.Value, Number);
			}
		}
#endif
	}

	// 에디터의 Instanced 클래스 드롭다운과 같은 조건 (SPropertyEditorEditInline)
	bool IsPickableInstancedClass(const UClass* Class)
	{
		if (!Class->HasAnyClassFlags(CLASS_EditInlineNew)
			|| Class->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated | CLASS_Abstract | CLASS_NewerVersionExists))
		{
			return false;
		}
		// BP 컴파일 중 생기는 임시 클래스
		const FString Name = Class->GetName();
		return !Name.StartsWith(TEXT("SKEL_")) && !Name.StartsWith(TEXT("REINST_"));
	}

	// Export 워커가 맵을 object 형태로 쓰는 키 타입 (키가 문자열로 나오는 것들)
	bool IsStringKeyType(const FString& Type)
	{
		using namespace DataBridge::SchemaTypes;
		return Type == String || Type == Name || Type == Text || Type == Enum
			|| Type == Tag || Type == Asset || Type == Class;
	}
}

FDataBridgeDescribeWalker::FDataBridgeDescribeWalker(FDataBridgeSchemaBuilder& InSchema, FDataBridgeContext& InCtx)
	: Schema(InSchema)
	, Ctx(InCtx)
{
}

void FDataBridgeDescribeWalker::DescribeClass(const UClass* Class)
{
	using namespace DataBridge;

	if (!Class || !Schema.TryBeginClass(Class))
	{
		return;
	}

	TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
	const FString ToolTip = DataBridgeDescribeWalkerPrivate::GetToolTip(Class);
	if (!ToolTip.IsEmpty())
	{
		Out->SetStringField(SchemaKeys::Description, ToolTip);
	}
	Out->SetObjectField(SchemaKeys::Fields, DescribeFields(Class));

	Schema.SetClass(Class, Out);
}

TSharedRef<FDataBridgeObject> FDataBridgeDescribeWalker::DescribeFields(const UStruct* Struct)
{
	using namespace DataBridge;

	TSharedRef<FDataBridgeObject> Fields = MakeShared<FDataBridgeObject>();

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Property = *It;
		if (!FDataBridgeExportWalker::ShouldWalk(Property))
		{
			continue;
		}

		// 지원하지 않는 타입은 Export/Import도 다루지 않으므로 설명서에서도 뺀다
		const TSharedPtr<FDataBridgeObject> Field = DescribeProperty(Property);
		if (!Field.IsValid())
		{
			continue;
		}

		const FString ToolTip = DataBridgeDescribeWalkerPrivate::GetToolTip(Property);
		if (ToolTip.IsEmpty())
		{
			// 상속받은 필드는 선언한 클래스 이름으로 한 번만
			const UStruct* Owner = Property->GetOwnerStruct();
			Schema.AddUndocumented((Owner ? Owner->GetName() : Struct->GetName()) + TEXT(".") + Property->GetName());
		}
		else
		{
			Field->SetStringField(SchemaKeys::Description, ToolTip);
		}

		Fields->SetObjectField(Property->GetName(), Field);
	}
	return Fields;
}

TSharedPtr<FDataBridgeObject> FDataBridgeDescribeWalker::DescribeProperty(const FProperty* Property)
{
	using namespace DataBridge;

	const TSharedPtr<FDataBridgeObject> Value = DescribeValue(Property);
	if (!Value.IsValid())
	{
		return nullptr;
	}

	// 메타는 필드(바깥 프로퍼티)에 붙는다. 배열이면 배열 설명에 같이 둔다
	DataBridgeDescribeWalkerPrivate::SetRange(*Value, Property);
#if WITH_EDITORONLY_DATA
	if (Property->HasMetaData(TEXT("Categories")))
	{
		Value->SetStringField(SchemaKeys::Categories, Property->GetMetaData(TEXT("Categories")));
	}
#endif

	// C 스타일 고정 배열 (int32 Values[4])
	if (Property->ArrayDim > 1)
	{
		TSharedRef<FDataBridgeObject> Array = DataBridgeDescribeWalkerPrivate::MakeType(SchemaTypes::Array);
		Array->SetObjectField(SchemaKeys::Items, Value);
		Array->SetNumberField(SchemaKeys::Size, Property->ArrayDim);
		return Array;
	}
	return Value;
}

TSharedPtr<FDataBridgeObject> FDataBridgeDescribeWalker::DescribeValue(const FProperty* Property)
{
	using namespace DataBridge;
	using DataBridgeDescribeWalkerPrivate::MakeType;

	if (CastField<FBoolProperty>(Property))
	{
		return MakeType(SchemaTypes::Bool);
	}
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		return DescribeEnum(EnumProperty->GetEnum());
	}
	if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property); ByteProperty && ByteProperty->Enum)
	{
		return DescribeEnum(ByteProperty->Enum);
	}
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		if (NumericProperty->IsFloatingPoint())
		{
			return MakeType(SchemaTypes::Float);
		}

		DataBridgeBitmask::FBitmaskInfo Bitmask;
		if (DataBridgeBitmask::Find(Property, Bitmask))
		{
			TArray<FString> Names;
			for (const DataBridgeBitmask::FFlag& Flag : Bitmask.Flags)
			{
				Names.Add(Flag.Name);
			}
			TSharedRef<FDataBridgeObject> Out = MakeType(SchemaTypes::Flags);
			DataBridgeDescribeWalkerPrivate::SetStringArray(*Out, SchemaKeys::Values, Names);
			Out->SetStringField(SchemaKeys::Note, TEXT("Array of flag names. [] means none."));
			return Out;
		}
		return MakeType(SchemaTypes::Int);
	}
	if (CastField<FStrProperty>(Property))
	{
		return MakeType(SchemaTypes::String);
	}
	if (CastField<FNameProperty>(Property))
	{
		return MakeType(SchemaTypes::Name);
	}
	if (CastField<FTextProperty>(Property))
	{
		return MakeType(SchemaTypes::Text);
	}
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		return DescribeStruct(StructProperty->Struct);
	}
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		const TSharedPtr<FDataBridgeObject> Inner = DescribeValue(ArrayProperty->Inner);
		if (!Inner.IsValid())
		{
			return nullptr;
		}
		TSharedRef<FDataBridgeObject> Out = MakeType(SchemaTypes::Array);
		Out->SetObjectField(SchemaKeys::Items, Inner);
		return Out;
	}
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		const TSharedPtr<FDataBridgeObject> Element = DescribeValue(SetProperty->ElementProp);
		if (!Element.IsValid())
		{
			return nullptr;
		}
		TSharedRef<FDataBridgeObject> Out = MakeType(SchemaTypes::Set);
		Out->SetObjectField(SchemaKeys::Items, Element);
		Out->SetStringField(SchemaKeys::Note, TEXT("Array without duplicates."));
		return Out;
	}
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		return DescribeMap(MapProperty);
	}
	if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
	{
		return DescribeObjectReference(ObjectProperty);
	}

	// Export/Import도 지원하지 않는 타입 (델리게이트, 인터페이스 등)
	return nullptr;
}

TSharedRef<FDataBridgeObject> FDataBridgeDescribeWalker::DescribeStruct(const UScriptStruct* Struct)
{
	using namespace DataBridge;

	// 표기법이 따로 등록된 struct는 포매터가 자기 모양을 말한다
	if (const IDataBridgeValueFormatter* Formatter = FDataBridgeRegistry::FindValueFormatter(Struct))
	{
		return Formatter->DescribeValue();
	}

	// 정의는 structs 테이블에 한 번만, 필드에서는 이름으로 가리킨다
	if (Schema.TryBeginStruct(Struct))
	{
		TSharedRef<FDataBridgeObject> Definition = MakeShared<FDataBridgeObject>();
		const FString ToolTip = DataBridgeDescribeWalkerPrivate::GetToolTip(Struct);
		if (!ToolTip.IsEmpty())
		{
			Definition->SetStringField(SchemaKeys::Description, ToolTip);
		}
		Definition->SetObjectField(SchemaKeys::Fields, DescribeFields(Struct));
		Schema.SetStruct(Struct, Definition);
	}

	TSharedRef<FDataBridgeObject> Out = DataBridgeDescribeWalkerPrivate::MakeType(SchemaTypes::Struct);
	Out->SetStringField(SchemaKeys::Struct, Struct->GetPathName());
	return Out;
}

TSharedPtr<FDataBridgeObject> FDataBridgeDescribeWalker::DescribeMap(const FMapProperty* Property)
{
	using namespace DataBridge;

	const TSharedPtr<FDataBridgeObject> Key = DescribeValue(Property->KeyProp);
	const TSharedPtr<FDataBridgeObject> Value = DescribeValue(Property->ValueProp);
	if (!Key.IsValid() || !Value.IsValid())
	{
		return nullptr;
	}

	const bool bObjectForm = DataBridgeDescribeWalkerPrivate::IsStringKeyType(Key->GetStringField(SchemaKeys::Type));

	TSharedRef<FDataBridgeObject> Out = DataBridgeDescribeWalkerPrivate::MakeType(SchemaTypes::Map);
	Out->SetObjectField(SchemaKeys::Key, Key);
	Out->SetObjectField(SchemaKeys::Value, Value);
	Out->SetStringField(SchemaKeys::MapForm, bObjectForm ? TEXT("object") : TEXT("pairs"));
	Out->SetStringField(SchemaKeys::Note, bObjectForm
		? TEXT("JSON object: each key is a map key, e.g. { \"<key>\": <value> }. Duplicate keys are errors.")
		: TEXT("Array of { \"key\": <key>, \"value\": <value> }. Duplicate keys are errors."));
	return Out;
}

TSharedPtr<FDataBridgeObject> FDataBridgeDescribeWalker::DescribeObjectReference(const FObjectPropertyBase* Property)
{
	using namespace DataBridge;
	using DataBridgeDescribeWalkerPrivate::MakeType;

	if (CastField<FWeakObjectProperty>(Property) || CastField<FLazyObjectProperty>(Property))
	{
		return nullptr;
	}

	// 클래스 참조 (TSoftClassPtr는 FSoftObjectProperty의 자식이라 먼저 본다)
	const UClass* MetaClass = nullptr;
	if (const FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
	{
		MetaClass = SoftClassProperty->MetaClass;
	}
	else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		MetaClass = ClassProperty->MetaClass;
	}
	if (MetaClass || CastField<FSoftClassProperty>(Property) || CastField<FClassProperty>(Property))
	{
		TSharedRef<FDataBridgeObject> Out = MakeType(SchemaTypes::Class);
		Out->SetStringField(SchemaKeys::Class, (MetaClass ? MetaClass : UObject::StaticClass())->GetPathName());
		Out->SetStringField(SchemaKeys::Note, TEXT("Class path (Blueprint classes end with _C), or null."));
		return Out;
	}

	// Instanced = 이 에셋이 소유하는 서브오브젝트
	if (Property->HasAnyPropertyFlags(CPF_InstancedReference) && !CastField<FSoftObjectProperty>(Property))
	{
		return DescribeInstanced(Property->PropertyClass);
	}

	// 하드/소프트 에셋 참조: 둘 다 경로 문자열
	TSharedRef<FDataBridgeObject> Out = MakeType(SchemaTypes::Asset);
	Out->SetStringField(SchemaKeys::Class, Property->PropertyClass->GetPathName());
	Out->SetStringField(SchemaKeys::Note, TEXT("Asset path such as \"/Game/Folder/Asset\", or null (see catalog.assets)."));
	Schema.AddAssetClass(Property->PropertyClass);
	return Out;
}

TSharedRef<FDataBridgeObject> FDataBridgeDescribeWalker::DescribeInstanced(const UClass* BaseClass)
{
	using namespace DataBridge;

	// 로드된 클래스만 찾는다 (C++ 클래스 + 지금 메모리에 있는 BP)
	TArray<UClass*> Candidates;
	GetDerivedClasses(BaseClass, Candidates, /*bRecursive*/ true);
	Candidates.Add(const_cast<UClass*>(BaseClass));
	Candidates.Sort([](const UClass& A, const UClass& B) { return A.GetPathName() < B.GetPathName(); });

	TArray<FString> Allowed;
	for (const UClass* Candidate : Candidates)
	{
		if (DataBridgeDescribeWalkerPrivate::IsPickableInstancedClass(Candidate))
		{
			Allowed.Add(Candidate->GetPathName());
			DescribeClass(Candidate);
		}
	}

	if (Allowed.Num() == 0)
	{
		Ctx.AddWarning(FString::Printf(TEXT("No pickable EditInlineNew class derives from '%s'"), *BaseClass->GetPathName()));
	}

	TSharedRef<FDataBridgeObject> Out = DataBridgeDescribeWalkerPrivate::MakeType(SchemaTypes::Instanced);
	Out->SetStringField(SchemaKeys::Class, BaseClass->GetPathName());
	DataBridgeDescribeWalkerPrivate::SetStringArray(*Out, SchemaKeys::Allowed, Allowed);
	Out->SetStringField(SchemaKeys::Note,
		TEXT("Object with \"$class\" set to one of the allowed classes, plus that class's fields (see classes). Omitted fields take the class defaults. Or null."));
	return Out;
}

TSharedRef<FDataBridgeObject> FDataBridgeDescribeWalker::DescribeEnum(const UEnum* Enum)
{
	using namespace DataBridge;

	TArray<FString> Names;
	// 마지막 항목은 컴파일러가 붙인 _MAX
	for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
	{
#if WITH_EDITORONLY_DATA
		if (Enum->HasMetaData(TEXT("Hidden"), Index))
		{
			continue;
		}
#endif
		Names.Add(Enum->GetNameStringByIndex(Index));
	}

	TSharedRef<FDataBridgeObject> Out = DataBridgeDescribeWalkerPrivate::MakeType(SchemaTypes::Enum);
	DataBridgeDescribeWalkerPrivate::SetStringArray(*Out, SchemaKeys::Values, Names);
	return Out;
}
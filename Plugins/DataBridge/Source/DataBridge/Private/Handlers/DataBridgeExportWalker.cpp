#include "Handlers/DataBridgeExportWalker.h"

#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Interfaces/DataBridgeValueFormatter.h"
#include "Internationalization/Text.h"
#include "UObject/EnumProperty.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/TextProperty.h"
#include "UObject/UnrealType.h"
#include "Handlers/DataBridgeBitmask.h"

namespace DataBridgeExportWalkerPrivate
{

	// 배열/맵 원소가 실패해도 인덱스가 밀리지 않게 null로 자리를 채운다
	TSharedPtr<FDataBridgeValue> OrNull(const TSharedPtr<FDataBridgeValue>& Value)
	{
		return Value.IsValid() ? Value : MakeShared<FJsonValueNull>();
	}
}

FDataBridgeExportWalker::FDataBridgeExportWalker(const FDataBridgeReferenceResolver& InResolver, FDataBridgeContext& InCtx)
	: Resolver(InResolver)
	, Ctx(InCtx)
{
}

TSharedPtr<FDataBridgeObject> FDataBridgeExportWalker::ExportObject(const UObject* Object)
{
	if (!Object)
	{
		Ctx.AddError(TEXT("Object is null"));
		return nullptr;
	}

	const TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
	ExportFieldsInto(*Out, Object->GetClass(), Object);
	return Out;
}

bool FDataBridgeExportWalker::ShouldWalk(const FProperty* Property)
{
	return Property->HasAnyPropertyFlags(CPF_Edit) 
		&& !Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_EditConst);

}

void FDataBridgeExportWalker::ExportFieldsInto(FDataBridgeObject& Out, const UStruct* Struct, const void* Container)
{
	bool bHasAnyProperty = false;

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Property = *It;
		bHasAnyProperty = true;
		if (!ShouldWalk(Property))
		{
			continue;
		}

		FDataBridgePathScope Scope(Ctx, Property->GetName());
		const TSharedPtr<FDataBridgeValue> Value = ExportProperty(Property, Property->ContainerPtrToValuePtr<void>(Container));
		if (Value.IsValid())
		{
			Out.SetField(Property->GetName(), Value);
		}
	}

	if (bHasAnyProperty && Out.Values.Num() == 0 && !Struct->IsA<UClass>())
	{
		Ctx.AddWarning(FString::Printf(TEXT("Struct '%s' has no editable fields; exported as an empty object"), *Struct->GetName()));
	}
}

TSharedPtr<FDataBridgeValue> FDataBridgeExportWalker::ExportProperty(const FProperty* Property, const void* ValuePtr)
{
	if (Property->ArrayDim == 1)
	{
		return ExportValue(Property, ValuePtr);
	}

	// C 스타일 고정 배열 (int32 Values[4])
	TArray<TSharedPtr<FDataBridgeValue>> Items;
	for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
	{
		FDataBridgePathScope Scope(Ctx, Index);
		const void* ElementPtr = static_cast<const uint8*>(ValuePtr) + Index * Property->GetElementSize();
		Items.Add(DataBridgeExportWalkerPrivate::OrNull(ExportValue(Property, ElementPtr)));
	}
	return MakeShared<FJsonValueArray>(Items);
}

TSharedPtr<FDataBridgeValue> FDataBridgeExportWalker::ExportValue(const FProperty* Property, const void* ValuePtr)
{
	if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		return MakeShared<FJsonValueBoolean>(BoolProperty->GetPropertyValue(ValuePtr));
	}
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		return ExportEnumValue(EnumProperty->GetEnum(), EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr));
	}
	if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property); ByteProperty && ByteProperty->Enum)
	{
		return ExportEnumValue(ByteProperty->Enum, ByteProperty->GetPropertyValue(ValuePtr));
	}
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		if (NumericProperty->IsFloatingPoint())
		{
			return MakeShared<FJsonValueNumber>(NumericProperty->GetFloatingPointPropertyValue(ValuePtr));
		}

		// JSON 숫자는 double 하나. int64는 2^53을 넘으면 정밀도가 깎인다 (게임 데이터 범위에선 문제없음)
		const int64 Value = NumericProperty->GetSignedIntPropertyValue(ValuePtr);

		DataBridgeBitmask::FBitmaskInfo Bitmask;
		if (DataBridgeBitmask::Find(Property, Bitmask))
		{
			return ExportBitmask(Bitmask, Property->GetElementSize(), Value);
		}
		return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
	}
	if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
	{
		return MakeShared<FJsonValueString>(StrProperty->GetPropertyValue(ValuePtr));
	}
	if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
	{
		return MakeShared<FJsonValueString>(NameProperty->GetPropertyValue(ValuePtr).ToString());
	}
	if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		// 현재 언어로 번역된 표시 문자열이 아니라 원문을 쓴다
		const FText& Text = TextProperty->GetPropertyValue(ValuePtr);
		const FString* Source = FTextInspector::GetSourceString(Text);
		return MakeShared<FJsonValueString>(Source ? *Source : Text.ToString());
	}
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		return ExportStruct(StructProperty->Struct, ValuePtr);
	}
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper Helper(ArrayProperty, ValuePtr);
		TArray<TSharedPtr<FDataBridgeValue>> Items;
		for (int32 Index = 0; Index < Helper.Num(); ++Index)
		{
			FDataBridgePathScope Scope(Ctx, Index);
			Items.Add(DataBridgeExportWalkerPrivate::OrNull(ExportValue(ArrayProperty->Inner, Helper.GetRawPtr(Index))));
		}
		return MakeShared<FJsonValueArray>(Items);
	}
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		FScriptSetHelper Helper(SetProperty, ValuePtr);
		TArray<TSharedPtr<FDataBridgeValue>> Items;
		for (int32 Index = 0, Count = 0; Index < Helper.GetMaxIndex(); ++Index)
		{
			if (!Helper.IsValidIndex(Index))
			{
				continue;
			}
			FDataBridgePathScope Scope(Ctx, Count++);
			Items.Add(DataBridgeExportWalkerPrivate::OrNull(ExportValue(SetProperty->ElementProp, Helper.GetElementPtr(Index))));
		}
		return MakeShared<FJsonValueArray>(Items);
	}
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		return ExportMap(MapProperty, ValuePtr);
	}
	if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
	{
		return ExportObjectReference(ObjectProperty, ValuePtr);
	}

	Ctx.AddWarning(FString::Printf(TEXT("Unsupported property type '%s', skipped"), *Property->GetClass()->GetName()));
	return nullptr;
}

TSharedPtr<FDataBridgeValue> FDataBridgeExportWalker::ExportStruct(const UScriptStruct* Struct, const void* ValuePtr)
{
	// 표기법이 따로 등록된 struct면 포매터에게 맡긴다 (인터페이스로만 호출)
	if (const IDataBridgeValueFormatter* Formatter = FDataBridgeRegistry::FindValueFormatter(Struct))
	{
		return Formatter->Export(ValuePtr, Ctx);
	}

	const TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
	ExportFieldsInto(*Out, Struct, ValuePtr);
	return MakeShared<FJsonValueObject>(Out);
}

TSharedPtr<FDataBridgeValue> FDataBridgeExportWalker::ExportMap(const FMapProperty* MapProperty, const void* ValuePtr)
{
	struct FPair
	{
		TSharedPtr<FDataBridgeValue> Key;
		TSharedPtr<FDataBridgeValue> Value;
	};

	FScriptMapHelper Helper(MapProperty, ValuePtr);
	TArray<FPair> Pairs;
	bool bAllStringKeys = true;

	for (int32 Index = 0, Count = 0; Index < Helper.GetMaxIndex(); ++Index)
	{
		if (!Helper.IsValidIndex(Index))
		{
			continue;
		}
		FDataBridgePathScope Scope(Ctx, Count++);
		FPair& Pair = Pairs.AddDefaulted_GetRef();
		Pair.Key = DataBridgeExportWalkerPrivate::OrNull(ExportValue(MapProperty->KeyProp, Helper.GetKeyPtr(Index)));
		Pair.Value = DataBridgeExportWalkerPrivate::OrNull(ExportValue(MapProperty->ValueProp, Helper.GetValuePtr(Index)));
		bAllStringKeys &= (Pair.Key->Type == EJson::String);
	}

	// 키가 전부 문자열이면 { "Stat.Attack": 100 }
	if (bAllStringKeys)
	{
		const TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
		for (const FPair& Pair : Pairs)
		{
			Out->SetField(Pair.Key->AsString(), Pair.Value);
		}
		return MakeShared<FJsonValueObject>(Out);
	}

	// 아니면 [ { "key": 4, "value": "four" } ]
	TArray<TSharedPtr<FDataBridgeValue>> Items;
	for (const FPair& Pair : Pairs)
	{
		const TSharedRef<FDataBridgeObject> Entry = MakeShared<FDataBridgeObject>();
		Entry->SetField(DataBridge::Keys::MapKey, Pair.Key);
		Entry->SetField(DataBridge::Keys::MapValue, Pair.Value);
		Items.Add(MakeShared<FJsonValueObject>(Entry));
	}
	return MakeShared<FJsonValueArray>(Items);
}

TSharedPtr<FDataBridgeValue> FDataBridgeExportWalker::ExportObjectReference(const FObjectPropertyBase* Property, const void* ValuePtr)
{
	if (CastField<FWeakObjectProperty>(Property) || CastField<FLazyObjectProperty>(Property))
	{
		Ctx.AddWarning(FString::Printf(TEXT("Unsupported reference type '%s', skipped"), *Property->GetClass()->GetName()));
		return nullptr;
	}

	// Soft 참조 (TSoftObjectPtr, TSoftClassPtr): 경로를 그대로, 재매핑하지 않는다
	if (const FSoftObjectProperty* SoftProperty = CastField<FSoftObjectProperty>(Property))
	{
		const FSoftObjectPath Path = SoftProperty->GetPropertyValuePtr(ValuePtr)->ToSoftObjectPath();
		if (Path.IsNull())
		{
			return MakeShared<FJsonValueNull>();
		}
		return MakeShared<FJsonValueString>(FDataBridgeReferenceResolver::ToPreferredForm(Path.ToString()));
	}

	const UObject* Object = Property->GetObjectPropertyValue(ValuePtr);
	if (!Object)
	{
		return MakeShared<FJsonValueNull>();
	}

	// Instanced = 이 에셋이 소유한 서브오브젝트 → 안으로 들어가서 펼친다
	if (Property->HasAnyPropertyFlags(CPF_InstancedReference))
	{
		return ExportInstanced(Object);
	}

	// 하드 참조 / 클래스 참조 → 주소록으로 주소 글자로
	return MakeShared<FJsonValueString>(Resolver.ToLogicalPath(Object));
}

TSharedPtr<FDataBridgeValue> FDataBridgeExportWalker::ExportInstanced(const UObject* Object)
{
	if (InstancedDepth >= DataBridge::MaxInstancedDepth)
	{
		Ctx.AddError(TEXT("Instanced objects are nested too deeply (possible cycle)"));
		return nullptr;
	}
	TGuardValue<int32> DepthGuard(InstancedDepth, InstancedDepth + 1);

	const TSharedRef<FDataBridgeObject> Out = MakeShared<FDataBridgeObject>();
	Out->SetStringField(DataBridge::Keys::InstancedClass, Resolver.ToLogicalPath(Object->GetClass()));
	ExportFieldsInto(*Out, Object->GetClass(), Object);
	return MakeShared<FJsonValueObject>(Out);
}

TSharedPtr<FDataBridgeValue> FDataBridgeExportWalker::ExportEnumValue(const UEnum* Enum, int64 Value)
{
	const FString Name = Enum->GetNameStringByValue(Value);
	if (Name.IsEmpty())
	{
		Ctx.AddWarning(FString::Printf(TEXT("Value %lld is not an entry of enum '%s'; exported as a number"), Value, *Enum->GetName()));
		return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
	}
	return MakeShared<FJsonValueString>(Name);
}

TSharedPtr<FDataBridgeValue> FDataBridgeExportWalker::ExportBitmask(const DataBridgeBitmask::FBitmaskInfo& Bitmask, int32 ByteSize, int64 Value)
{
	// 프로퍼티 크기만큼만 본다. int32에서 31번 비트가 켜지면 음수가 되어 위쪽 비트가 부호 확장되므로
	const uint64 SizeMask = ByteSize >= 8 ? MAX_uint64 : (uint64(1) << (ByteSize * 8)) - 1;
	uint64 Remaining = static_cast<uint64>(Value) & SizeMask;

	TArray<TSharedPtr<FDataBridgeValue>> Names;
	for (const DataBridgeBitmask::FFlag& Flag : Bitmask.Flags)
	{
		if ((Remaining & Flag.Bit) != 0)
		{
			Names.Add(MakeShared<FJsonValueString>(Flag.Name));
			Remaining &= ~Flag.Bit;
		}
	}

	// 이름 없는 비트가 남으면 정보를 잃지 않게 숫자로 (읽기 불편 < 데이터 손실)
	if (Remaining != 0)
	{
		Ctx.AddWarning(FString::Printf(TEXT("Bitmask has bits that '%s' does not name (0x%llX); exported as a number to keep them"),
			*Bitmask.Enum->GetName(), Remaining));
		return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
	}
	return MakeShared<FJsonValueArray>(Names);
}

#include "Handlers/DataBridgeImportWalker.h"

#include "DataBridgeReferenceResolver.h"
#include "DataBridgeRegistry.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Handlers/DataBridgeExportWalker.h"
#include "Interfaces/DataBridgeValueFormatter.h"
#include "Internationalization/Text.h"
#include "UObject/EnumProperty.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/TextProperty.h"
#include "UObject/UnrealType.h"
#include "Handlers/DataBridgeBitMask.h"

namespace DataBridgeImportWalkerPrivate
{

	// Json 타입 이름을 문자열로 반환. 디버그용
	const TCHAR* JsonTypeName(EJson Type)
	{
		switch (Type)
		{
		case EJson::Null: return TEXT("null");
		case EJson::String: return TEXT("string");
		case EJson::Number: return TEXT("number");
		case EJson::Boolean: return TEXT("boolean");
		case EJson::Array: return TEXT("array");
		case EJson::Object: return TEXT("object");
		default: return TEXT("none");
		}
	}

	// Value.Type이 기대 타입과 같은지 확인. 다르면 에러 기록 후 false 반환
	bool ExpectType(const FDataBridgeValue& Value, EJson Expected, FDataBridgeContext& Ctx)
	{
		if (Value.Type == Expected)
		{
			return true;
		}
		Ctx.AddError(FString::Printf(TEXT("Expected %s, got %s"), JsonTypeName(Expected), JsonTypeName(Value.Type)));
		return false;
	}
	
	// 정수형 Property의 최소값과 최대값을 반환
	void GetIntegerRange(const FNumericProperty* Property, double& OutMin, double& OutMax)
	{
		if		(Property->IsA<FInt8Property>())	{ OutMin = MIN_int8;  OutMax = MAX_int8; }
		else if (Property->IsA<FInt16Property>())	{ OutMin = MIN_int16; OutMax = MAX_int16; }
		else if (Property->IsA<FIntProperty>())		{ OutMin = MIN_int32; OutMax = MAX_int32; }
		else if (Property->IsA<FByteProperty>())	{ OutMin = 0;         OutMax = MAX_uint8; }
		else if (Property->IsA<FUInt16Property>())	{ OutMin = 0;         OutMax = MAX_uint16; }
		else if (Property->IsA<FUInt32Property>())	{ OutMin = 0;         OutMax = MAX_uint32; }
		else if (Property->IsA<FUInt64Property>())	{ OutMin = 0;         OutMax = static_cast<double>(MAX_uint64); }
		else										{ OutMin = static_cast<double>(MIN_int64); OutMax = static_cast<double>(MAX_int64); }
	}

	// 에디터의 ClampMin/ClampMax. 잘라서 넣지 않고 에러로 알린다 (AI가 틀린 값을 성공으로 착각하지 않게).
// 메타는 필드에 직접 붙은 경우만 보인다 (배열 원소는 바깥 배열에 붙으므로 검사 안 됨)
	bool CheckClamp(const FProperty* Property, double Number, FDataBridgeContext& Ctx)
	{
#if WITH_EDITORONLY_DATA
		double Limit = 0.0;
		if (Property->HasMetaData(TEXT("ClampMin")) && LexTryParseString(Limit, *Property->GetMetaData(TEXT("ClampMin"))) && Number < Limit)
		{
			Ctx.AddError(FString::Printf(TEXT("%g is below ClampMin %g"), Number, Limit));
			return false;
		}
		if (Property->HasMetaData(TEXT("ClampMax")) && LexTryParseString(Limit, *Property->GetMetaData(TEXT("ClampMax"))) && Number > Limit)
		{
			Ctx.AddError(FString::Printf(TEXT("%g is above ClampMax %g"), Number, Limit));
			return false;
		}
#endif
		return true;
	}

	// 맵 키/Set 원소를 임시로 만들어 둘 메모리 (생성자에서 초기화, 소멸자에서 파괴)
	class FScopedPropertyValue : public FNoncopyable
	{
	public:
		explicit FScopedPropertyValue(const FProperty* InProperty)
			: Property(InProperty)
			, Data(FMemory::Malloc(InProperty->GetSize(), InProperty->GetMinAlignment()))
		{
			Property->InitializeValue(Data);
		}
		~FScopedPropertyValue()
		{
			Property->DestroyValue(Data);
			FMemory::Free(Data);
		}
		void* Get() const { return Data; }

	private:
		const FProperty* Property;
		void* Data;
	};
}

FDataBridgeImportWalker::FDataBridgeImportWalker(const FDataBridgeReferenceResolver& InResolver, FDataBridgeContext& InCtx)
	: Resolver(InResolver)
	, Ctx(InCtx)
{
}

bool FDataBridgeImportWalker::ImportObject(const FDataBridgeObject& Data, UObject* Target)
{
	if (!Target)
	{
		Ctx.AddError(TEXT("Target is null"));
		return false;
	}

	const int32 ErrorsBefore = Ctx.GetErrorCount();
	TGuardValue<UObject*> OuterGuard(CurrentOuter, Target);
	ImportFields(Data, Target->GetClass(), Target, /*bSkipInstancedClassKey*/ false);
	return Ctx.GetErrorCount() == ErrorsBefore;
}

void FDataBridgeImportWalker::ImportFields(const FDataBridgeObject& Json, const UStruct* Struct, void* Container, bool bSkipInstancedClassKey)
{
	for (const TPair<FString, TSharedPtr<FDataBridgeValue>>& Pair : Json.Values)
	{
		if (bSkipInstancedClassKey && Pair.Key == DataBridge::Keys::InstancedClass)
		{
			continue;
		}

		FDataBridgePathScope Scope(Ctx, Pair.Key);

		const FProperty* Property = FindFProperty<FProperty>(Struct, *Pair.Key);
		if (!Property)
		{
			// 오타를 조용히 무시하면 AI는 바꿨다고 믿는다 → 에러
			Ctx.AddError(FString::Printf(TEXT("Unknown field '%s' on '%s'"), *Pair.Key, *Struct->GetName()));
			continue;
		}
		if (!FDataBridgeExportWalker::ShouldWalk(Property))
		{
			Ctx.AddError(TEXT("Field is not editable (read-only, transient or deprecated)"));
			continue;
		}
		if (!Pair.Value.IsValid())
		{
			Ctx.AddError(TEXT("Value is missing"));
			continue;
		}

		ImportProperty(Property, *Pair.Value, Property->ContainerPtrToValuePtr<void>(Container));
	}
}

bool FDataBridgeImportWalker::ImportProperty(const FProperty* Property, const FDataBridgeValue& Value, void* ValuePtr)
{
	if (Property->ArrayDim == 1)
	{
		return ImportValue(Property, Value, ValuePtr);
	}

	// C 스타일 고정 배열: 개수가 정확히 맞아야 한다
	if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::Array, Ctx))
	{
		return false;
	}
	const TArray<TSharedPtr<FDataBridgeValue>>& Items = Value.AsArray();
	if (Items.Num() != Property->ArrayDim)
	{
		Ctx.AddError(FString::Printf(TEXT("Expected exactly %d elements, got %d"), Property->ArrayDim, Items.Num()));
		return false;
	}

	// 배열 원소를 하나씩 처리
	bool bOk = true;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		FDataBridgePathScope Scope(Ctx, Index);
		void* ElementPtr = static_cast<uint8*>(ValuePtr) + Index * Property->GetElementSize();
		bOk &= Items[Index].IsValid() && ImportValue(Property, *Items[Index], ElementPtr);
	}
	return bOk;
}

bool FDataBridgeImportWalker::ImportValue(const FProperty* Property, const FDataBridgeValue& Value, void* ValuePtr)
{
	if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
	{
		if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::Boolean, Ctx))
		{
			return false;
		}
		BoolProperty->SetPropertyValue(ValuePtr, Value.AsBool());
		return true;
	}
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		int64 EnumValue = 0;
		if (!ImportEnum(EnumProperty->GetEnum(), Value, EnumValue))
		{
			return false;
		}
		EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, EnumValue);
		return true;
	}
	if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property); ByteProperty && ByteProperty->Enum)
	{
		int64 EnumValue = 0;
		if (!ImportEnum(ByteProperty->Enum, Value, EnumValue))
		{
			return false;
		}
		ByteProperty->SetPropertyValue(ValuePtr, static_cast<uint8>(EnumValue));
		return true;
	}
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		return ImportNumeric(NumericProperty, Value, ValuePtr);
	}
	if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
	{
		if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::String, Ctx))
		{
			return false;
		}
		StrProperty->SetPropertyValue(ValuePtr, Value.AsString());
		return true;
	}
	if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
	{
		if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::String, Ctx))
		{
			return false;
		}
		NameProperty->SetPropertyValue(ValuePtr, FName(*Value.AsString()));
		return true;
	}
	if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		return ImportText(TextProperty, Value, ValuePtr);
	}
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		return ImportStruct(StructProperty->Struct, Value, ValuePtr);
	}
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		return ImportArray(ArrayProperty, Value, ValuePtr);
	}
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		return ImportSet(SetProperty, Value, ValuePtr);
	}
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		return ImportMap(MapProperty, Value, ValuePtr);
	}
	if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
	{
		return ImportObjectReference(ObjectProperty, Value, ValuePtr);
	}

	Ctx.AddError(FString::Printf(TEXT("Unsupported property type '%s'"), *Property->GetClass()->GetName()));
	return false;
}

bool FDataBridgeImportWalker::ImportNumeric(const FNumericProperty* Property, const FDataBridgeValue& Value, void* ValuePtr)
{
	// 비트마스크는 이름 배열도 받는다.
	// 숫자도 계속 허용: 이름 없는 비트가 있어 숫자로 Export된 경우의 왕복용
	if (!Property->IsFloatingPoint())
	{
		DataBridgeBitmask::FBitmaskInfo Bitmask;
		if (DataBridgeBitmask::Find(Property, Bitmask))
		{
			if (Value.Type == EJson::Array)
			{
				return ImportBitmask(Property, Bitmask, Value, ValuePtr);
			}
			if (Value.Type != EJson::Number)
			{
				Ctx.AddError(FString::Printf(TEXT("Expected an array of '%s' flag names (or a number), got %s"),
					*Bitmask.Enum->GetName(), DataBridgeImportWalkerPrivate::JsonTypeName(Value.Type)));
				return false;
			}
		}
	}

	if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::Number, Ctx))
	{
		return false;
	}

	const double Number = Value.AsNumber();
	if (Property->IsFloatingPoint())
	{
		if (!DataBridgeImportWalkerPrivate::CheckClamp(Property, Number, Ctx))
		{
			return false;
		}
		Property->SetFloatingPointPropertyValue(ValuePtr, Number);
		return true;
	}

	if (Number != FMath::RoundToDouble(Number))
	{
		Ctx.AddError(FString::Printf(TEXT("Expected an integer, got %g"), Number));
		return false;
	}

	double Min = 0.0, Max = 0.0;
	DataBridgeImportWalkerPrivate::GetIntegerRange(Property, Min, Max);
	if (Number < Min || Number > Max)
	{
		Ctx.AddError(FString::Printf(TEXT("%.0f is out of range [%.0f, %.0f]"), Number, Min, Max));
		return false;
	}

	if (!DataBridgeImportWalkerPrivate::CheckClamp(Property, Number, Ctx))
	{
		return false;
	}

	if (Property->IsA<FUInt64Property>())
	{
		Property->SetIntPropertyValue(ValuePtr, static_cast<uint64>(Number));
	}
	else
	{
		Property->SetIntPropertyValue(ValuePtr, static_cast<int64>(Number));
	}
	return true;
}

bool FDataBridgeImportWalker::ImportBitmask(const FNumericProperty* Property, const DataBridgeBitmask::FBitmaskInfo& Bitmask, const FDataBridgeValue& Value, void* ValuePtr)
{
	const TArray<TSharedPtr<FDataBridgeValue>>& Items = Value.AsArray();
	uint64 Bits = 0;
	bool bOk = true;

	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		FDataBridgePathScope Scope(Ctx, Index);

		if (!Items[Index].IsValid() || !DataBridgeImportWalkerPrivate::ExpectType(*Items[Index], EJson::String, Ctx))
		{
			bOk = false;
			continue;
		}

		// "Enemy"와 "ESkillTargetFlags::Enemy" 모두 허용 (일반 enum과 같은 규칙)
		FString Name = Items[Index]->AsString();
		FString EnumPrefix, ShortName;
		if (Name.Split(TEXT("::"), &EnumPrefix, &ShortName))
		{
			Name = ShortName;
		}

		const DataBridgeBitmask::FFlag* Flag = Bitmask.Flags.FindByPredicate([&Name](const DataBridgeBitmask::FFlag& Candidate)
			{
				return Candidate.Name.Equals(Name, ESearchCase::IgnoreCase);
			});

		if (!Flag)
		{
			TArray<FString> Names;
			for (const DataBridgeBitmask::FFlag& Each : Bitmask.Flags)
			{
				Names.Add(Each.Name);
			}
			Ctx.AddError(FString::Printf(TEXT("'%s' is not a flag of '%s'. Use any of: %s"),
				*Items[Index]->AsString(), *Bitmask.Enum->GetName(), *FString::Join(Names, TEXT(", "))));
			bOk = false;
			continue;
		}

		// 문자열이 아니라 비트로 판정: "Enemy"와 "ESkillTargetFlags::Enemy"도 중복
		if ((Bits & Flag->Bit) != 0)
		{
			Ctx.AddError(FString::Printf(TEXT("Duplicate flag '%s'"), *Flag->Name));
			bOk = false;
			continue;
		}
		Bits |= Flag->Bit;
	}

	if (!bOk)
	{
		return false;
	}

	// uint64로 넣으면 프로퍼티 크기에 맞게 잘린다. 플래그는 Find에서 크기 안의 것만 골랐으므로 손실 없음
	Property->SetIntPropertyValue(ValuePtr, Bits);
	return true;
}

bool FDataBridgeImportWalker::ImportEnum(const UEnum* Enum, const FDataBridgeValue& Value, int64& OutValue)
{
	if (Value.Type == EJson::String)
	{
		// "WhenDamaged"와 "EHealthBarDisplay::WhenDamaged" 모두 허용
		const int64 Found = Enum->GetValueByNameString(Value.AsString());
		if (Found != INDEX_NONE)
		{
			OutValue = Found;
			return true;
		}

		TArray<FString> Names;
		for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
		{
			const FString Name = Enum->GetNameStringByIndex(Index);
			if (!Name.EndsWith(TEXT("_MAX")) && Name != TEXT("MAX") && !Enum->HasMetaData(TEXT("Hidden"), Index))
			{
				Names.Add(Name);
			}
		}
		Ctx.AddError(FString::Printf(TEXT("'%s' is not a value of enum '%s'. Use one of: %s"),
			*Value.AsString(), *Enum->GetName(), *FString::Join(Names, TEXT(", "))));
		return false;
	}

	// Export가 이름 없는 값을 숫자로 내보낸 경우의 왕복용
	if (Value.Type == EJson::Number)
	{
		const int64 Number = static_cast<int64>(Value.AsNumber());
		if (Enum->IsValidEnumValue(Number))
		{
			OutValue = Number;
			return true;
		}
	}

	Ctx.AddError(FString::Printf(TEXT("Expected a value name of enum '%s'"), *Enum->GetName()));
	return false;
}

bool FDataBridgeImportWalker::ImportText(const FTextProperty* Property, const FDataBridgeValue& Value, void* ValuePtr)
{
	if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::String, Ctx))
	{
		return false;
	}

	FText& Text = *Property->GetPropertyValuePtr(ValuePtr);
	const FString NewSource = Value.AsString();

	// 원문이 그대로면 손대지 않는다 → 기존 현지화 키 유지
	const FString* CurrentSource = FTextInspector::GetSourceString(Text);
	if (CurrentSource && CurrentSource->Equals(NewSource, ESearchCase::CaseSensitive))
	{
		return true;
	}

	// 바뀌었으면 기존 네임스페이스/키를 유지하고 원문만 바꾼다 (디테일 패널 편집과 같은 동작). 키가 없으면 새로 만든다.
	const TOptional<FString> Namespace = FTextInspector::GetNamespace(Text);
	const TOptional<FString> Key = FTextInspector::GetKey(Text);
	const FString NamespaceString = Namespace.Get(FString());
	const FString KeyString = (Key.IsSet() && !Key->IsEmpty()) ? *Key : FGuid::NewGuid().ToString();

	Text = FText::AsLocalizable_Advanced(FTextKey(*NamespaceString), FTextKey(*KeyString), FStringView(NewSource));
	return true;
}

bool FDataBridgeImportWalker::ImportStruct(const UScriptStruct* Struct, const FDataBridgeValue& Value, void* ValuePtr)
{
	// 표기법이 등록된 struct면 포매터에게 맡긴다
	if (const IDataBridgeValueFormatter* Formatter = FDataBridgeRegistry::FindValueFormatter(Struct))
	{
		return Formatter->Import(Value, ValuePtr, Ctx);
	}

	if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::Object, Ctx))
	{
		return false;
	}

	// 일반 struct는 기존 값 위에 적힌 필드만 덮어쓴다
	const int32 ErrorsBefore = Ctx.GetErrorCount();
	ImportFields(*Value.AsObject(), Struct, ValuePtr, /*bSkipInstancedClassKey*/ false);
	return Ctx.GetErrorCount() == ErrorsBefore;
}

bool FDataBridgeImportWalker::ImportArray(const FArrayProperty* Property, const FDataBridgeValue& Value, void* ValuePtr)
{
	if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::Array, Ctx))
	{
		return false;
	}

	const TArray<TSharedPtr<FDataBridgeValue>>& Items = Value.AsArray();
	FScriptArrayHelper Helper(Property, ValuePtr);
	Helper.EmptyAndAddValues(Items.Num());   // 통째 교체: 기본값 원소로 채운 뒤 하나씩 덮어쓴다

	bool bOk = true;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		FDataBridgePathScope Scope(Ctx, Index);
		if (!Items[Index].IsValid())
		{
			Ctx.AddError(TEXT("Value is missing"));
			bOk = false;
			continue;
		}
		bOk &= ImportValue(Property->Inner, *Items[Index], Helper.GetRawPtr(Index));
	}
	return bOk;
}

bool FDataBridgeImportWalker::ImportSet(const FSetProperty* Property, const FDataBridgeValue& Value, void* ValuePtr)
{
	if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::Array, Ctx))
	{
		return false;
	}

	const TArray<TSharedPtr<FDataBridgeValue>>& Items = Value.AsArray();
	FScriptSetHelper Helper(Property, ValuePtr);
	Helper.EmptyElements();

	bool bOk = true;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		FDataBridgePathScope Scope(Ctx, Index);

		DataBridgeImportWalkerPrivate::FScopedPropertyValue Element(Property->ElementProp);
		if (!Items[Index].IsValid() || !ImportValue(Property->ElementProp, *Items[Index], Element.Get()))
		{
			bOk = false;
			continue;
		}

		const int32 CountBefore = Helper.Num();
		Helper.AddElement(Element.Get());
		if (Helper.Num() == CountBefore)
		{
			Ctx.AddError(TEXT("Duplicate element in a set"));
			bOk = false;
		}
	}
	return bOk;
}

bool FDataBridgeImportWalker::ImportMap(const FMapProperty* Property, const FDataBridgeValue& Value, void* ValuePtr)
{
	FScriptMapHelper Helper(Property, ValuePtr);

	auto AddPair = [this, Property, &Helper](const FDataBridgeValue& KeyJson, const FDataBridgeValue& ValueJson) -> bool
		{
			DataBridgeImportWalkerPrivate::FScopedPropertyValue Key(Property->KeyProp);
			DataBridgeImportWalkerPrivate::FScopedPropertyValue Val(Property->ValueProp);

			bool bOk = ImportValue(Property->KeyProp, KeyJson, Key.Get());
			bOk &= ImportValue(Property->ValueProp, ValueJson, Val.Get());
			if (!bOk)
			{
				return false;
			}

			const int32 CountBefore = Helper.Num();
			Helper.AddPair(Key.Get(), Val.Get());
			if (Helper.Num() == CountBefore)
			{
				Ctx.AddError(TEXT("Duplicate key in a map"));
				return false;
			}
			return true;
		};

	// 형식 1: { "Stat.Attack": 100 }  (키가 문자열로 표현되는 맵)
	if (Value.Type == EJson::Object)
	{
		Helper.EmptyValues();
		bool bOk = true;
		for (const TPair<FString, TSharedPtr<FDataBridgeValue>>& Pair : Value.AsObject()->Values)
		{
			FDataBridgePathScope Scope(Ctx, Pair.Key);
			if (!Pair.Value.IsValid())
			{
				Ctx.AddError(TEXT("Value is missing"));
				bOk = false;
				continue;
			}
			bOk &= AddPair(FJsonValueString(Pair.Key), *Pair.Value);
		}
		return bOk;
	}

	// 형식 2: [ { "key": 4, "value": "four" } ]
	if (Value.Type == EJson::Array)
	{
		Helper.EmptyValues();
		bool bOk = true;
		const TArray<TSharedPtr<FDataBridgeValue>>& Items = Value.AsArray();
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			FDataBridgePathScope Scope(Ctx, Index);

			const TSharedPtr<FDataBridgeObject>* Entry = nullptr;
			if (!Items[Index].IsValid() || !Items[Index]->TryGetObject(Entry))
			{
				Ctx.AddError(TEXT("Expected { \"key\": ..., \"value\": ... }"));
				bOk = false;
				continue;
			}

			const TSharedPtr<FDataBridgeValue> KeyJson = (*Entry)->TryGetField(DataBridge::Keys::MapKey);
			const TSharedPtr<FDataBridgeValue> ValueJson = (*Entry)->TryGetField(DataBridge::Keys::MapValue);
			if (!KeyJson.IsValid() || !ValueJson.IsValid())
			{
				Ctx.AddError(TEXT("Each pair needs both \"key\" and \"value\""));
				bOk = false;
				continue;
			}
			bOk &= AddPair(*KeyJson, *ValueJson);
		}
		return bOk;
	}

	Ctx.AddError(FString::Printf(TEXT("Expected object or array of pairs, got %s"), DataBridgeImportWalkerPrivate::JsonTypeName(Value.Type)));
	return false;
}

bool FDataBridgeImportWalker::ImportObjectReference(const FObjectPropertyBase* Property, const FDataBridgeValue& Value, void* ValuePtr)
{
	if (CastField<FWeakObjectProperty>(Property) || CastField<FLazyObjectProperty>(Property))
	{
		Ctx.AddError(FString::Printf(TEXT("Unsupported reference type '%s'"), *Property->GetClass()->GetName()));
		return false;
	}

	// Soft 참조: 존재만 확인하고 경로를 그대로 저장 (재매핑 안 함)
	if (const FSoftObjectProperty* SoftProperty = CastField<FSoftObjectProperty>(Property))
	{
		FSoftObjectPtr& SoftPtr = *SoftProperty->GetPropertyValuePtr(ValuePtr);
		if (Value.Type == EJson::Null)
		{
			SoftPtr = FSoftObjectPtr();
			return true;
		}
		if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::String, Ctx))
		{
			return false;
		}

		const FString Path = FDataBridgeReferenceResolver::NormalizeObjectPath(Value.AsString());
		if (Path.IsEmpty())
		{
			SoftPtr = FSoftObjectPtr();
			return true;
		}
		if (!Resolver.CheckSoft(Path, Ctx))
		{
			return false;
		}
		SoftPtr = FSoftObjectPtr(FSoftObjectPath(Path));
		return true;
	}

	if (Value.Type == EJson::Null)
	{
		Property->SetObjectPropertyValue(ValuePtr, nullptr);
		return true;
	}

	// Instanced = 이 에셋이 소유하는 서브오브젝트 → 새로 만든다
	if (Property->HasAnyPropertyFlags(CPF_InstancedReference))
	{
		return ImportInstanced(Property, Value, ValuePtr);
	}

	if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::String, Ctx))
	{
		return false;
	}
	const FString Text = Value.AsString();

	// 클래스 참조 (TSubclassOf 등): 짧은 이름도 허용, 기반 클래스 검사
	if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		UClass* Class = Text.StartsWith(TEXT("/"))
			? Cast<UClass>(Resolver.ResolveHard(Text, UClass::StaticClass(), Ctx))
			: FDataBridgeReferenceResolver::FindClass(Text, Ctx);
		if (!Class)
		{
			return false;
		}
		if (ClassProperty->MetaClass && !Class->IsChildOf(ClassProperty->MetaClass))
		{
			Ctx.AddError(FString::Printf(TEXT("'%s' is not a subclass of '%s'"), *Class->GetName(), *ClassProperty->MetaClass->GetName()));
			return false;
		}
		Property->SetObjectPropertyValue(ValuePtr, Class);
		return true;
	}

	// 하드 참조: 주소록이 찾아준다 (연습 = 복제본, 실전 = 진짜)
	UObject* Object = Resolver.ResolveHard(Text, Property->PropertyClass, Ctx);
	if (!Object)
	{
		return false;
	}
	Property->SetObjectPropertyValue(ValuePtr, Object);
	return true;
}

bool FDataBridgeImportWalker::ImportInstanced(const FObjectPropertyBase* Property, const FDataBridgeValue& Value, void* ValuePtr)
{
	if (!DataBridgeImportWalkerPrivate::ExpectType(Value, EJson::Object, Ctx))
	{
		return false;
	}
	const FDataBridgeObject& Json = *Value.AsObject();

	// $class 해석과 검사
	UClass* Class = nullptr;
	{
		FDataBridgePathScope ClassScope(Ctx, DataBridge::Keys::InstancedClass);

		FString ClassText;
		if (!Json.TryGetStringField(DataBridge::Keys::InstancedClass, ClassText))
		{
			Ctx.AddError(TEXT("Missing required field"));
			return false;
		}

		Class = FDataBridgeReferenceResolver::FindClass(ClassText, Ctx);
		if (!Class)
		{
			return false;
		}
		if (!Class->IsChildOf(Property->PropertyClass))
		{
			Ctx.AddError(FString::Printf(TEXT("'%s' is not a subclass of '%s'"), *Class->GetName(), *Property->PropertyClass->GetName()));
			return false;
		}
		if (Class->HasAnyClassFlags(CLASS_Abstract))
		{
			Ctx.AddError(FString::Printf(TEXT("'%s' is abstract"), *Class->GetName()));
			return false;
		}
		if (!Class->HasAnyClassFlags(CLASS_EditInlineNew))
		{
			Ctx.AddError(FString::Printf(TEXT("'%s' is not EditInlineNew, so it cannot be created inline"), *Class->GetName()));
			return false;
		}
	}

	if (InstancedDepth >= DataBridge::MaxInstancedDepth)
	{
		Ctx.AddError(TEXT("Instanced objects are nested too deeply"));
		return false;
	}
	TGuardValue<int32> DepthGuard(InstancedDepth, InstancedDepth + 1);

	// 통째로 새로: 클래스 기본값에서 시작해 적힌 필드만 채운다
	check(CurrentOuter);
	const EObjectFlags Flags = CurrentOuter->GetMaskedFlags(RF_PropagateToSubObjects) | RF_Transactional;
	UObject* NewObject = ::NewObject<UObject>(CurrentOuter, Class, NAME_None, Flags);
	CreatedObjects.Add(NewObject);
	Property->SetObjectPropertyValue(ValuePtr, NewObject);

	const int32 ErrorsBefore = Ctx.GetErrorCount();
	{
		TGuardValue<UObject*> OuterGuard(CurrentOuter, NewObject);   // 안쪽 Instanced의 주인은 이 오브젝트
		ImportFields(Json, Class, NewObject, /*bSkipInstancedClassKey*/ true);
	}
	return Ctx.GetErrorCount() == ErrorsBefore;
}
#include "Handlers/DataBridgeBitmask.h"

#include "UObject/Class.h"
#include "UObject/UnrealType.h"

bool DataBridgeBitmask::Find(const FProperty* Property, FBitmaskInfo& OutInfo)
{
	OutInfo = FBitmaskInfo();

#if WITH_EDITORONLY_DATA
	const FNumericProperty* Numeric = CastField<FNumericProperty>(Property);
	if (!Numeric || !Numeric->IsInteger() || !Property->HasMetaData(TEXT("Bitmask")))
	{
		return false;
	}

	const FString& EnumName = Property->GetMetaData(TEXT("BitmaskEnum"));
	const UEnum* Enum = EnumName.IsEmpty() ? nullptr : UClass::TryFindTypeSlow<UEnum>(EnumName);
	if (!Enum)
	{
		return false;
	}

	const bool bValuesAreMasks = Enum->GetBoolMetaData(TEXT("UseEnumValuesAsMaskValuesInEditor"));
	const int32 BitCount = Property->GetElementSize() * 8;

	OutInfo.Enum = Enum;

	// 마지막 항목은 컴파일러가 붙인 _MAX
	for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
	{
		if (Enum->HasMetaData(TEXT("Hidden"), Index))
		{
			continue;
		}

		const int64 Value = Enum->GetValueByIndex(Index);
		uint64 Bit = 0;

		if (bValuesAreMasks)
		{
			// 값이 곧 비트. 2의 거듭제곱만 플래그로 인정 (0이나 조합값은 제외)
			if (Value <= 0 || (Value & (Value - 1)) != 0 || (BitCount < 64 && Value >= (int64(1) << BitCount)))
			{
				continue;
			}
			Bit = static_cast<uint64>(Value);
		}
		else
		{
			// 값이 비트 번호
			if (Value < 0 || Value >= BitCount)
			{
				continue;
			}
			Bit = uint64(1) << Value;
		}

		OutInfo.Flags.Add({ Enum->GetNameStringByIndex(Index), Bit });
	}
	return true;
#else
	return false;
#endif
}
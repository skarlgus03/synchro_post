#pragma once

#include "CoreMinimal.h"

class FProperty;
class UEnum;

// 비트 마스크 정수 필드 (meta = Bitmask,BitmaksEnum) <-> 플래그 이름 목록.
// 에디터 디테일 패널 (SPropertyEditorNumeric) 과 같은 규칙으로 해석한다.
namespace DataBridgeBitmask
{
	struct FFlag
	{
		FString Name;	// 짧은 이름 ("Enemy")
		uint64 Bit = 0;	// 실제 비트 값
	};

	struct FBitmaskInfo
	{
		const UEnum* Enum = nullptr;
		TArray<FFlag> Flags; // enum 정의 순서
	};

	// 이름 붙은 비트마스크 필드면 OutInfo를 채우고 true
	// Bitmask 메타만 있고 BitmaskEnum이 없거나, enum을 못 찾으면 false (그냥 숫자로 함)
	bool Find(const FProperty* Property, FBitmaskInfo& OutInfo);
}

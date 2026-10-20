#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "NativeGameplayTags.h"

class UDataBridgeTestAllTypes;

// 테스트 전용 태그 (프로젝트 태그에 의존하지 않기 위해 플러그인 안에서 정의)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_DataBridgeTest_A);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_DataBridgeTest_B);

namespace DataBridgeTestFixtures
{
	// /Temp/<Folder>/<Prefix>_<Guid>
	FString UniqueTempPackageName(const TCHAR* Folder, const TCHAR* Prefix);

	// 빈 AllTypes 오브젝트 (/Temp에 새 패키지, 이름 = 패키지 짧은 이름)
	UDataBridgeTestAllTypes* MakeEmptyAllTypes(const TCHAR* Prefix);

	// 모든 필드를 채운 AllTypes 오브젝트
	UDataBridgeTestAllTypes* MakeFilledAllTypes();
}

#endif
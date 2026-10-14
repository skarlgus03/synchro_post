#pragma once

#include "CoreMinimal.h"

namespace StatMath
{
	constexpr int32 PercentScale = 100;

	// 방어 상수, 방어력 공식: 100 / (100 + 방어력)
	constexpr int32 DefenseScale = 100;

	FORCEINLINE float PercentToFloat(int32 Value)
	{
		return static_cast<float>(Value) / PercentScale;
	}

	FORCEINLINE float PercentToMultiplier(int32 Value)
	{
		return 1.0f + PercentToFloat(Value);
	}

	// 관통 후 실제 방어력. 비율 관통 먼저, 고정 관통 나중. 0 밑으로 안 내려감
	FORCEINLINE int32 EffectiveDefense(int32 Defense, int32 PenPercent, int32 PenFlat)
	{
		const float AfterPercent = static_cast<float>(Defense) * (1.f - PercentToFloat(PenPercent));
		return FMath::Max(FMath::RoundToInt32(AfterPercent) - PenFlat, 0);
	}

	// 방어력 → 피해 배율 (방어 0 = 1배, DefenseScale = 0.5배)
	FORCEINLINE float DefenseToMultiplier(int32 Defense)
	{
		return DefenseScale / (DefenseScale + static_cast<float>(Defense));
	}

	// 저항(%) → 피해 배율. 100 이상이면 0(면역), 음수(약점)면 1 초과
	FORCEINLINE float ResistanceToMultiplier(int32 ResistancePercent)
	{
		return FMath::Max(0.f, 1.f - PercentToFloat(ResistancePercent));
	}
}
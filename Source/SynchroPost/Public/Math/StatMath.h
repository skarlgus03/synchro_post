#pragma once

#include "CoreMinimal.h"

namespace StatMath
{
	constexpr int32 PercentScale = 100;

	FORCEINLINE float PercentToFloat(int32 Value)
	{
		return static_cast<float>(Value) / PercentScale;
	}

	FORCEINLINE float PercentToMultiplier(int32 Value)
	{
		return 1.0f + PercentToFloat(Value);
	}
}
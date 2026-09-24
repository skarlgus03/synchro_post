#pragma once

#include "CoreMinimal.h"

namespace GridMath
{
	FORCEINLINE FIntPoint RotateOffset (const FIntPoint& Offset, const FIntPoint& Direction)
	{
		if (Direction == FIntPoint(0, 1)) // North
		{
			return Offset;
		}
		else if (Direction == FIntPoint(1, 0)) // East
		{
			return FIntPoint(Offset.Y, -Offset.X);
		}
		else if (Direction == FIntPoint(0, -1)) // South
		{
			return FIntPoint(-Offset.X, -Offset.Y);
		}
		else if (Direction == FIntPoint(-1, 0)) // West
		{
			return FIntPoint(-Offset.Y, Offset.X);
		}
		return Offset; // Default case, no rotation
	}

	FORCEINLINE FIntPoint ToCardinalDirection(const FIntPoint& Delta)
	{
		if (FMath::Abs(Delta.X) > FMath::Abs(Delta.Y))
		{
			return FIntPoint(FMath::Sign(Delta.X), 0); 
		}
		else
		{
			return FIntPoint(0, FMath::Sign(Delta.Y)); 
		}
	}
}
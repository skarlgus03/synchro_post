#include "Math/DamageMath.h"

int32 DamageMath::ResolveDamage(FSPHealthActionData& Data, FDefenseFn DefenseMultiplier, FResistanceFn ResistanceMultiplier)
{
	Data.EnsureParts();

	int32 Total = 0;
	int32 LargestIndex = INDEX_NONE;   // 반올림 전 가장 큰 조각 (최소 피해를 줄 곳)
	float LargestScaled = 0.f;

	for (int32 i = 0; i < Data.Parts.Num(); ++i)
	{
		FDamagePart& Part = Data.Parts[i];
		const float Scaled = static_cast<float>(Part.Amount)
			* DefenseMultiplier(Part.Tags, Data.PenetrationData)
			* ResistanceMultiplier(Part.Tags);

		if (Scaled > LargestScaled)
		{
			LargestScaled = Scaled;
			LargestIndex = i;
		}

		Part.Amount = FMath::Max(0, FMath::RoundToInt32(Scaled));
		Total += Part.Amount;
	}

	// 최소 피해 1은 타격 전체에 한 번만. 전부 면역(배율 0)이면 0 그대로
	if (Total == 0 && LargestIndex != INDEX_NONE)
	{
		Data.Parts[LargestIndex].Amount = 1;
		Total = 1;
	}

	Data.SyncTotalsFromParts();
	return Total;
}
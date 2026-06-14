#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"


namespace SPTags
{
	namespace Stat
	{
		namespace Combat
		{
			namespace Primary
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxHealth);

				UE_DECLARE_GAMEPLAY_TAG_EXTERN(AtkPhysical);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(AtkMagic);

				UE_DECLARE_GAMEPLAY_TAG_EXTERN(DefPhysical);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(DefMagic);

				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Speed);
			}
			namespace Secondary
			{
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxAP);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(MaxBP);

				namespace Penetration
				{
					UE_DECLARE_GAMEPLAY_TAG_EXTERN(PhysicalFlat);
					UE_DECLARE_GAMEPLAY_TAG_EXTERN(PhysicalPercent);
					UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicFlat);
					UE_DECLARE_GAMEPLAY_TAG_EXTERN(MagicPercent);
				}

				UE_DECLARE_GAMEPLAY_TAG_EXTERN(CritChance);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(CritDamage);

				UE_DECLARE_GAMEPLAY_TAG_EXTERN(LifeSteal);

				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Accuracy);
				UE_DECLARE_GAMEPLAY_TAG_EXTERN(Evasion);
			}

		}
	}

	namespace Source
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff);
	}
}
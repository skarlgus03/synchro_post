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

	namespace Damage
	{
		namespace Form
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Physical);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Magic);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(True);
		}
		namespace Type
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slash);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Pierce);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Blunt);
		}
		namespace Element
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Fire);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ice);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Electric);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Light);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dark);
		}
	}
}
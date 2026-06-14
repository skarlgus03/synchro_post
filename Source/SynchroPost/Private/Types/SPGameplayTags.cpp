
#include "Types/SPGameplayTags.h"

namespace SPTags
{
	namespace Stat
	{
		namespace Combat
		{
			namespace Primary
			{
				UE_DEFINE_GAMEPLAY_TAG(MaxHealth, "Stat.Combat.Primary.MaxHealth");

				UE_DEFINE_GAMEPLAY_TAG(AtkPhysical, "Stat.Combat.Primary.AtkPhysical");
				UE_DEFINE_GAMEPLAY_TAG(AtkMagic, "Stat.Combat.Primary.AtkMagic");

				UE_DEFINE_GAMEPLAY_TAG(DefPhysical, "Stat.Combat.Primary.DefPhysical");
				UE_DEFINE_GAMEPLAY_TAG(DefMagic, "Stat.Combat.Primary.DefMagic");

				UE_DEFINE_GAMEPLAY_TAG(Speed, "Stat.Combat.Primary.Speed");
			}
			namespace Secondary
			{
				UE_DEFINE_GAMEPLAY_TAG(MaxAP, "Stat.Combat.Secondary.MaxAP");
				UE_DEFINE_GAMEPLAY_TAG(MaxBP, "Stat.Combat.Secondary.MaxBP");

				namespace Penetration
				{
					UE_DEFINE_GAMEPLAY_TAG(PhysicalFlat, "Stat.Combat.Secondary.Penetration.PhysicalFlat");
					UE_DEFINE_GAMEPLAY_TAG(PhysicalPercent, "Stat.Combat.Secondary.Penetration.PhysicalPercent");
					UE_DEFINE_GAMEPLAY_TAG(MagicFlat, "Stat.Combat.Secondary.Penetration.MagicFlat");
					UE_DEFINE_GAMEPLAY_TAG(MagicPercent, "Stat.Combat.Secondary.Penetration.MagicPercent");
				}

				UE_DEFINE_GAMEPLAY_TAG(CritChance, "Stat.Combat.Secondary.CritChance");
				UE_DEFINE_GAMEPLAY_TAG(CritDamage, "Stat.Combat.Secondary.CritDamage");

				UE_DEFINE_GAMEPLAY_TAG(LifeSteal, "Stat.Combat.Secondary.LifeSteal");

				UE_DEFINE_GAMEPLAY_TAG(Accuracy, "Stat.Combat.Secondary.Accuracy");
				UE_DEFINE_GAMEPLAY_TAG(Evasion, "Stat.Combat.Secondary.Evasion");
			}
		}
	}

	namespace Source
	{
		UE_DEFINE_GAMEPLAY_TAG(Equipment, "Source.Equipment");
		UE_DEFINE_GAMEPLAY_TAG(Slot, "Source.Slot");
		UE_DEFINE_GAMEPLAY_TAG(Buff, "Source.Buff");
	}
}


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
		namespace Item
		{
			namespace Equipment
			{
				UE_DEFINE_GAMEPLAY_TAG(Weapon, "Source.Item.Equipment.Weapon");
				namespace Armor
				{
					UE_DEFINE_GAMEPLAY_TAG(Head, "Source.Item.Equipment.Armor.Head");
					UE_DEFINE_GAMEPLAY_TAG(Chest, "Source.Item.Equipment.Armor.Chest");
					UE_DEFINE_GAMEPLAY_TAG(Hands, "Source.Item.Equipment.Armor.Hands");
					UE_DEFINE_GAMEPLAY_TAG(Feet, "Source.Item.Equipment.Armor.Feet");
				}
				namespace Accessory
				{
					UE_DEFINE_GAMEPLAY_TAG(Ring, "Source.Item.Equipment.Accessory.Ring");
					UE_DEFINE_GAMEPLAY_TAG(Necklace, "Source.Item.Equipment.Accessory.Necklace");
				}
			}
			namespace Consumable
			{
				UE_DEFINE_GAMEPLAY_TAG(Potion, "Source.Item.Consumable.Potion");
			}
		}
	}

	namespace Damage
	{
		namespace Form
		{
			UE_DEFINE_GAMEPLAY_TAG(Physical, "Damage.Form.Physical");
			UE_DEFINE_GAMEPLAY_TAG(Magic, "Damage.Form.Magic");
			UE_DEFINE_GAMEPLAY_TAG(True, "Damage.Form.True");
		}
		namespace Type
		{
			UE_DEFINE_GAMEPLAY_TAG(Slash, "Damage.Type.Slash");
			UE_DEFINE_GAMEPLAY_TAG(Pierce, "Damage.Type.Pierce");
			UE_DEFINE_GAMEPLAY_TAG(Blunt, "Damage.Type.Blunt");
		}
		namespace Element
		{
			UE_DEFINE_GAMEPLAY_TAG(Fire, "Damage.Element.Fire");
			UE_DEFINE_GAMEPLAY_TAG(Ice, "Damage.Element.Ice");
			UE_DEFINE_GAMEPLAY_TAG(Electric, "Damage.Element.Electric");
			UE_DEFINE_GAMEPLAY_TAG(Light, "Damage.Element.Light");
			UE_DEFINE_GAMEPLAY_TAG(Dark, "Damage.Element.Dark");
		}
	}

	namespace Item
	{
		namespace Equipment
		{
			UE_DEFINE_GAMEPLAY_TAG(Weapon, "Item.Equipment.Weapon");
			namespace Armor
			{
				UE_DEFINE_GAMEPLAY_TAG(Head, "Item.Equipment.Armor.Head");
				UE_DEFINE_GAMEPLAY_TAG(Chest, "Item.Equipment.Armor.Chest");
				UE_DEFINE_GAMEPLAY_TAG(Hands, "Item.Equipment.Armor.Hands");
				UE_DEFINE_GAMEPLAY_TAG(Feet, "Item.Equipment.Armor.Feet");
			}
			namespace Accessory
			{
				UE_DEFINE_GAMEPLAY_TAG(Ring, "Item.Equipment.Accessory.Ring");
				UE_DEFINE_GAMEPLAY_TAG(Necklace, "Item.Equipment.Accessory.Necklace");
			}
		}
		namespace Consumable
		{
			UE_DEFINE_GAMEPLAY_TAG(Potion, "Item.Consumable.Potion");
		}
		UE_DEFINE_GAMEPLAY_TAG(Special, "Item.Special");
	}
	namespace Skill
	{
		namespace Resource
		{
			UE_DEFINE_GAMEPLAY_TAG(AP, "Skill.Resource.AP");
			UE_DEFINE_GAMEPLAY_TAG(BP, "Skill.Resource.BP");
		}
		namespace Slot
		{
			namespace Unit
			{
				UE_DEFINE_GAMEPLAY_TAG(Passive, "Skill.Slot.Unit.Passive");
				UE_DEFINE_GAMEPLAY_TAG(Skill1, "Skill.Slot.Unit.Skill1");
				UE_DEFINE_GAMEPLAY_TAG(Skill2, "Skill.Slot.Unit.Skill2");
				UE_DEFINE_GAMEPLAY_TAG(Skill3, "Skill.Slot.Unit.Skill3");
				UE_DEFINE_GAMEPLAY_TAG(Ultimate, "Skill.Slot.Unit.Ultimate");
			}
		}
	}
	namespace Tile
	{
		namespace Effect
		{
			UE_DEFINE_GAMEPLAY_TAG(Wall, "Tile.Effect.Wall");
		}
	}
	namespace StatusEffect
	{
		namespace DoT
		{
			UE_DEFINE_GAMEPLAY_TAG(Burn, "StatusEffect.DoT.Burn");
			UE_DEFINE_GAMEPLAY_TAG(Poison, "StatusEffect.DoT.Poison");
			UE_DEFINE_GAMEPLAY_TAG(Bleed, "StatusEffect.DoT.Bleed");
		}
	}
}

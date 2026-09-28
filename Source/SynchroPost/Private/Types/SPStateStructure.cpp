#include "Types/SPStateStructure.h"
#include "StatusEffect/StatusEffectDataAsset.h"


FGameplayTag FStateTagEntry::GetTag() const
{
	if (DataAsset)
	{
		return DataAsset->StatusEffectTag;
	}
	return FGameplayTag();
}
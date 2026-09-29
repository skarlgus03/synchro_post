

#include "StatusEffect/StatusEffect_Buff.h"
#include "Unit/StateComponent.h"
#include "Unit/StatComponent.h"
#include "Unit/Unit.h"



void UStatusEffect_Buff::OnApply_Implementation(const FStateTagEntry& Entry)
{
	AUnit* OwnerUnit = Cast<AUnit>(OwnerComp->GetOwner());
	TArray<FStatModifierEntry> ModifierEntries;
	if (UStatComponent* StatComp = OwnerUnit->GetStatComponent())
	{
		for (const FStatModifier& Modifier : StatModifiers)
		{
			FStatModifierEntry ModEntry;
			ModEntry.Source = this;
			ModEntry.StatModifier = Modifier;
			ModifierEntries.Add(ModEntry);
		}
		StatComp->AddStatusEffectModifiers(ModifierEntries);
	}
}

void UStatusEffect_Buff::OnRemove_Implementation(const FStateTagEntry& Entry)
{
	AUnit* OwnerUnit = Cast<AUnit>(OwnerComp->GetOwner());
	if (UStatComponent* StatComp = OwnerUnit->GetStatComponent())
	{
		StatComp->RemoveStatusEffectModifiers(this);
	}

}

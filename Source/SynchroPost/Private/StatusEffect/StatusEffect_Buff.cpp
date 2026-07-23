

#include "StatusEffect/StatusEffect_Buff.h"
#include "Unit/StateComponent.h"
#include "Unit/StatComponent.h"
#include "Unit/Unit.h"

void UStatusEffect_Buff::InitializeBuff(AUnit* InSource, const TArray<FStatModifier>& NewModifiers)
{
	SourceUnit = InSource;

	for (const FStatModifier& Modifier : NewModifiers)
	{
		FStatModifierEntry Entry;
		Entry.StatModifier = Modifier;
		Entry.Source = this;
		StatModifiers.Add(Entry);
	}
}

void UStatusEffect_Buff::OnApply_Implementation()
{
	AUnit* OwnerUnit = Cast<AUnit>(OwnerComp->GetOwner());
	if (UStatComponent* StatComp = OwnerUnit->GetStatComponent())
	{
		StatComp->AddStatusEffectModifiers(StatModifiers);
	}
}

void UStatusEffect_Buff::OnRemove_Implementation()
{
	AUnit* OwnerUnit = Cast<AUnit>(OwnerComp->GetOwner());
	if (UStatComponent* StatComp = OwnerUnit->GetStatComponent())
	{
		StatComp->RemoveStatusEffectModifiers(this);
	}

}

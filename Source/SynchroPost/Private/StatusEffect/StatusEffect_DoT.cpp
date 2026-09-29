
#include "StatusEffect/StatusEffect_DoT.h"
#include "Unit/StateComponent.h"
#include "Unit/Unit.h"
#include "Interface/Damageable.h"

void UStatusEffect_DoT::InitializeDoT(AUnit* InSource, int32 InDamagePerTurn, const FGameplayTagContainer& InActionTypeTagss)
{
	Source = InSource;
	DamagePerTurn = InDamagePerTurn;
	ActionTypeTags = InActionTypeTagss;
}

void UStatusEffect_DoT::OnTurnEnd_Implementation(const FStateTagEntry& Entry)
{
	AUnit* OwnerUnit = Cast<AUnit>(OwnerComp->GetOwner());
	if (!OwnerUnit)
	{
		return;
	}

	// Deal damage to the owner unit
	FSPHealthActionData ActionData;
	ActionData.Causer = Source.Get();
	ActionData.Amount = DamagePerTurn;
	ActionData.ActionTypeTags = ActionTypeTags;

	IDamageable::Execute_ApplyHealthChange(OwnerUnit, ActionData);
}

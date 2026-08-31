
#include "StatusEffect/StatusEffect_DoT.h"
#include "Unit/StateComponent.h"
#include "Unit/Unit.h"

void UStatusEffect_DoT::InitializeDoT(AUnit* InSource, int32 InDamagePerTurn, const FGameplayTagContainer& InActionTypeTagss)
{
	SourceUnit = InSource;
	DamagePerTurn = InDamagePerTurn;
	ActionTypeTagss = InActionTypeTagss;
}

void UStatusEffect_DoT::OnTurnEnd_Implementation()
{
	AUnit* OwnerUnit = Cast<AUnit>(OwnerComp->GetOwner());
	if (!OwnerUnit)
	{
		return;
	}

	// Deal damage to the owner unit
	FSPHealthActionData ActionData;
	ActionData.DamageCauser = SourceUnit.Get();
	ActionData.Amount = DamagePerTurn;
	ActionData.ActionTypeTags = ActionTypeTagss;

	OwnerUnit->ApplyHealthChange(ActionData);
}


#include "StatusEffect/StatusEffect_DoT.h"
#include "Unit/StateComponent.h"
#include "Unit/Unit.h"

void UStatusEffect_DoT::InitializeDoT(AUnit* InSource, int32 InDamagePerTurn)
{
	SourceUnit = InSource;
	DamagePerTurn = InDamagePerTurn;
}

void UStatusEffect_DoT::OnTurnEnd_Implementation()
{
	AUnit* OwnerUnit = Cast<AUnit>(OwnerComp->GetOwner());
	if (!OwnerUnit)
	{
		return;
	}

	// Deal damage to the owner unit

}

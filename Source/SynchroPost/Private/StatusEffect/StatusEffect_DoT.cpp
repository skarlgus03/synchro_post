
#include "StatusEffect/StatusEffect_DoT.h"
#include "Unit/StateComponent.h"
#include "Unit/Unit.h"

void UStatusEffect_DoT::InitializeDoT(AUnit* InSource, int32 InDamagePerTurn, const FGameplayTagContainer& InDamageTypeTags)
{
	SourceUnit = InSource;
	DamagePerTurn = InDamagePerTurn;
	DamageTypeTags = InDamageTypeTags;
}

void UStatusEffect_DoT::OnTurnEnd_Implementation()
{
	AUnit* OwnerUnit = Cast<AUnit>(OwnerComp->GetOwner());
	if (!OwnerUnit)
	{
		return;
	}

	// Deal damage to the owner unit
	FSPDamageData DamageData;
	DamageData.DamageCauser = SourceUnit.Get();
	DamageData.RawDamage = DamagePerTurn;
	DamageData.DamageTypeTags = DamageTypeTags;

	OwnerUnit->ApplyDamage(DamageData);
}

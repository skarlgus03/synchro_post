#pragma once

#include "CoreMinimal.h"
#include "StatusEffect/StatusEffectBase.h"
#include "GameplayTagContainer.h"
#include "Types/SynchroPostTypes.h"
#include "StatusEffect_Buff.generated.h"

UCLASS()
class SYNCHROPOST_API UStatusEffect_Buff : public UStatusEffectBase
{
	GENERATED_BODY()
	
public:
	
	virtual void OnApply_Implementation(const FStateTagEntry& Entry) override;
	virtual void OnRemove_Implementation(const FStateTagEntry& Entry) override;

protected:

	UPROPERTY(EditAnywhere, Category = "Status Effect")
	TArray<FStatModifier> StatModifiers;
};

#pragma once

#include "CoreMinimal.h"
#include "StatusEffect/StatusEffectBase.h"
#include "GameplayTagContainer.h"
#include "Types/SynchroPostTypes.h"
#include "StatusEffect_Buff.generated.h"

UCLASS(Abstract)
class SYNCHROPOST_API UStatusEffect_Buff : public UStatusEffectBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Status Effect")
	void InitializeBuff(AUnit* InSource, const TArray<FStatModifier>& NewModifiers);

	virtual void OnApply_Implementation() override;
	virtual void OnRemove_Implementation() override;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Status Effect")
	TArray<FStatModifierEntry> StatModifiers;
};

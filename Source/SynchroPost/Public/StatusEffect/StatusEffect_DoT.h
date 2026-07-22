#pragma once

#include "CoreMinimal.h"
#include "StatusEffect/StatusEffectBase.h"
#include "GameplayTagContainer.h"
#include "StatusEffect_DoT.generated.h"

UCLASS(Abstract)
class SYNCHROPOST_API UStatusEffect_DoT : public UStatusEffectBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Status Effect")
	void InitializeDoT(AUnit* InSource, int32 InDamagePerTurn, const FGameplayTagContainer& InDamageTypeTags);

	virtual void OnTurnEnd_Implementation() override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect")
	int32 DamagePerTurn;

	UPROPERTY(BlueprintReadOnly, Category = "Status Effect")
	FGameplayTagContainer DamageTypeTags;
};

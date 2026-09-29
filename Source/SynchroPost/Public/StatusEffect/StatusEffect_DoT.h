#pragma once

#include "CoreMinimal.h"
#include "StatusEffect/StatusEffectBase.h"
#include "GameplayTagContainer.h"
#include "StatusEffect_DoT.generated.h"

UENUM(BlueprintType)
enum class EDoTDamageType : uint8
{
	PerStat UMETA(DisplayName = "스탯 비례"),
	Fixed UMETA(DisplayName = "고정 피해량"),
	PerHealth UMETA(DisplayName = "체력 비례"),
};

UCLASS(Abstract)
class SYNCHROPOST_API UStatusEffect_DoT : public UStatusEffectBase
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Status Effect")
	void InitializeDoT(AUnit* InSource, int32 InDamagePerTurn, const FGameplayTagContainer& InActionTypeTags);

	virtual void OnTurnEnd_Implementation(const FStateTagEntry& Entry) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect")
	int32 DamagePerTurn;

	UPROPERTY(BlueprintReadOnly, Category = "Status Effect")
	FGameplayTagContainer ActionTypeTags;
};

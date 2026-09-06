

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/SynchroPostTypes.h"
#include "GameplayTagContainer.h"

#include "Damageable.generated.h"

UINTERFACE(MinimalAPI)
class UDamageable : public UInterface
{
	GENERATED_BODY()
};

class SYNCHROPOST_API IDamageable
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damageable")
	int32 ApplyHealthChange(const FSPHealthActionData& ActionData);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damageable")
	int32 GetCurrentHealth() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damageable")
	void ApplyVisualDamage(int32 DisplayAmount, int32 NewTargetHealth, bool bIsCritical, const FGameplayTagContainer& TypeTags);
};

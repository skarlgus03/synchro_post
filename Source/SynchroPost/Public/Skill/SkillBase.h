#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SynchroPostTypes.h"
#include "Types/SPSkillStructure.h"
#include "SkillBase.generated.h"


class AUnit;
class USkillDataAsset;
class USkillComponent;

UCLASS(Abstract, BlueprintType, EditInlineNew)
class SYNCHROPOST_API USkillBase : public UObject
{
	GENERATED_BODY()
	
public:

	// Query Functions

	int32 GetCurrentCooldown(const FGameplayTagContainer& StateTags) const;

	const FSkillData& GetSkillDataByIndex(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Skill")
	const FSkillData& GetCurrentSkillData(const FGameplayTagContainer& StateTags) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	int32 DetermineCurrentIndex(const FGameplayTagContainer& StateTags) const;

	virtual int32 DetermineCurrentIndex_Implementation(const FGameplayTagContainer& StateTags) const
	{
		return 0;
	}


public:

	void InitializeSkill(USkillDataAsset* InSkillDataAsset);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	void ExecuteSkill(AUnit* Caster, const TArray<AUnit*>& Target);

	virtual void ExecuteSkill_Implementation(AUnit* Caster, const TArray<AUnit*>& Target);

	
	// Checking Skill's Special Conditions
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	bool CheckSkillCondition(const FGameplayTagContainer& StateTags) const;

	virtual bool CheckSkillCondition_Implementation(const FGameplayTagContainer& StateTags) const
	{
		return true;
	}

	void DecreaseCooldowns();

	virtual bool IsSupportedForNetworking() const override { return true; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	TObjectPtr<USkillDataAsset> SkillDataAsset;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Skill Cooldown")
	TArray<int32> CurrentCooldown;

	TObjectPtr<USkillComponent> OwnerComp;
};

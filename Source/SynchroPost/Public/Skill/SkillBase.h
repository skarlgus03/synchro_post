#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SynchroPostTypes.h"
#include "SkillBase.generated.h"


class AUnit;

USTRUCT(BlueprintType)
struct FSkillData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FText SkillName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UTexture2D> SkillIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	int32 BaseCooldown = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	int32 APCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	int32 BPCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FSkillTargetingData TargetingData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	int32 TargetCount = 1;
};

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class SYNCHROPOST_API USkillBase : public UObject
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill Form Override")
	TArray<FSkillData> SkillDataArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill Cooldown")
	TArray<int32> CurrentCooldown;

	int32 CurrentStateIndex = 0;

public:
	
	void InitializeSkill();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	void ExecuteSkill(AUnit* Caster, const TArray<AUnit*>& Target);

	virtual void ExecuteSkill_Implementation(AUnit* Caster, const TArray<AUnit*>& Target);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void UpdateCurrentIndex(AUnit* Caster);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	int32 DetermineCurrentIndex(AUnit* Caster) const;

	virtual int32 DetermineCurrentIndex_Implementation(AUnit* Caster) const
	{
		return 0;
	}


	UFUNCTION(BlueprintCallable, Category = "Skill")
	const FSkillData& GetCurrentSkillData(AUnit* Caster) const;


	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool CanExecuteSkill(AUnit* Caster, const TArray<AUnit*>& Target) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	bool CanExecuteSkill_BP(AUnit* Caster, const TArray<AUnit*>& Target) const;

	virtual bool CanExecuteSkill_BP_Implementation(AUnit* Caster, const TArray<AUnit*>& Target) const
	{
		return true;
	}

	void DecreaseCooldowns();
};

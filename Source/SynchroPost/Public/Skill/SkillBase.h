#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SynchroPostTypes.h"
#include "Types/SPSkillStructure.h"
#include "SkillBase.generated.h"


class AUnit;
class USkillDataAsset;
class USkillComponent;

UCLASS(Abstract, BlueprintType, EditInlineNew, Blueprintable)
class SYNCHROPOST_API USkillBase : public UObject
{
	GENERATED_BODY()
	
public:

	// 내부 헬퍼 함수들

	int32 GetCurrentCooldown(const FGameplayTagContainer& StateTags) const;

	const FSkillData& GetSkillDataByIndex(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Skill")
	const FSkillData& GetCurrentSkillData(const FGameplayTagContainer& StateTags) const;

	const FSkillTargetingRule& GetTargetingRule(const FGameplayTagContainer& StateTags) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	int32 DetermineCurrentIndex(const FGameplayTagContainer& StateTags) const;

	virtual int32 DetermineCurrentIndex_Implementation(const FGameplayTagContainer& StateTags) const
	{
		return 0;
	}

	// 주인의 위치와, 타겟의 위치가 사거리 내인지 확인하는 함수
	bool IsWithinCastRange(const FIntPoint& TargetCoord, const int32& CastRange, const FSkillExecutionContext& Context) const;

	// 주인의 팩션과 타겟의 팩션을 확인해서 사용가능한지 판단하는 함수
	bool MatchesFaction(AUnit* Target, ESkillTargetFaction TargetFaction, const FSkillExecutionContext& Context) const;

public:

	void InitializeSkill(USkillDataAsset* InSkillDataAsset);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	void ExecuteSkill(AUnit* Caster, const TArray<AUnit*>& Target);

	virtual void ExecuteSkill_Implementation(AUnit* Caster, const TArray<AUnit*>& Target);


	// === 외부에서 호출될 조건 검사 함수들 ===

	// 스킬이 타겟에 대해 실행 가능한지 확인하는 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	bool CanExecuteOnTarget(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context) const;

	virtual bool CanExecuteOnTarget_Implementation(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context) const;

	// 스킬에 있는 특수한 조건을 확인하는 함수.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	bool CheckSkillCondition(const FSkillExecutionContext& Context) const;

	virtual bool CheckSkillCondition_Implementation(const FSkillExecutionContext& Context) const
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

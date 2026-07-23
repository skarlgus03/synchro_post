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
	bool MatchesFaction(ESkillTargetFaction TargetFaction, const FSkillExecutionContext& Context, EFaction TargetUnitFaction) const;

public:

	void InitializeSkill(USkillDataAsset* InSkillDataAsset);

	void SetOwnerComponent(USkillComponent* InOwnerComp) { OwnerComp = InOwnerComp; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	void ExecuteSkill(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context);

	virtual void ExecuteSkill_Implementation(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context);


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

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USkillComponent> OwnerComp;

protected:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	void ApplyEffectToTargets(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context) const;

	virtual void ApplyEffectToTargets_Implementation(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context) const
	{
		// 기본 구현은 아무것도 하지 않음. 필요에 따라 서브클래스에서 오버라이드 가능.
	}
};

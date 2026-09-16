#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Types/SynchroPostTypes.h"
#include "Types/SPCombatEventStructure.h"
#include "Types/SPSkillStructure.h"
#include "SkillBase.generated.h"


class AUnit;
class USkillDataAsset;
class USkillComponent;
class USkillPresentation;

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

	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool IsValidSingleTargetTile(const FIntPoint& Coord, const FSkillExecutionContext& Context) const;

	void InitializeSkill(USkillDataAsset* InSkillDataAsset);

	void SetOwnerComponent(USkillComponent* InOwnerComp) { OwnerComp = InOwnerComp; }


	// === 이벤트 관련 ===

	// 컴뱃 이벤트를 푸시한다.
	UFUNCTION(BlueprintCallable , Category = "Skill")
	void PushSkillCombatEvent(const FSkillExecutionContext& Context, const TArray<FCombatEventTarget>& Targets) ;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	void PresentSkillEffect(const FCombatEvent& Event);
	virtual void PresentSkillEffect_Implementation(const FCombatEvent& Event);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void NotifySkillEffectPresentationFinished() const;


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

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	TArray<FIntPoint> GetAffectedTiles(const FIntPoint& TargetCoord, const FSkillExecutionContext& Context) const;

	TArray<FIntPoint> GetAffectedTiles_Implementation(const FIntPoint& TargetCoord, const FSkillExecutionContext& Context) const;

	// 스킬이 사거리 내에 있는 타일들의 좌표를 반환합니다.
	UFUNCTION(BlueprintCallable, Category = "Skill")
	TArray<FIntPoint> GetRangeTiles(const FSkillExecutionContext& Context) const;

	// 스킬이 실행 가능한 타일들의 좌표를 반환합니다.
	UFUNCTION(BlueprintCallable, Category = "Skill")
	TArray<FIntPoint> GetValidTargetTiles(const FSkillExecutionContext& Context) const;



	void DecreaseCooldowns();


	// ──────── BP에서 스킬 로직 짤때 사용할 헬퍼 함수들 ────────


	UFUNCTION(BlueprintCallable, Category = "Skill|Helper")
	AUnit* GetOwnerUnit() const;

	/** 선택 타일들의 영향 범위에서 유닛을 모은다. 중복 제거 + 타겟팅 룰의 진영 필터 적용 */
	UFUNCTION(BlueprintCallable, Category = "Skill|Helper")
	TArray<AUnit*> GatherAffectedUnits(const FSkillTargetData& TargetData,
		const FSkillExecutionContext& Context);

	/** DamageCoefficients × 시전자 스탯을 합산. 방어/저항은 타겟이 처리하므로 여기선 제외 */
	UFUNCTION(BlueprintCallable, Category = "Skill|Helper")
	int32 CalculateSkillAmount(const FSkillExecutionContext& Context);

	/** 적용 전후 HP와 좌표를 채워 결과 하나를 만든다. 이 경로로만 데미지를 넣을 것 */
	UFUNCTION(BlueprintCallable, Category = "Skill|Helper")
	FCombatEventTarget ApplyToTarget(AUnit* TargetUnit, const FSPHealthActionData& ActionData);

	/** 평범한 데미지/회복 스킬의 전체 처리. 결과 배열을 반환한다 */
	UFUNCTION(BlueprintCallable, Category = "Skill|Helper")
	TArray<FCombatEventTarget> ApplyStandardEffect(const FSkillTargetData& TargetData,
		const FSkillExecutionContext& Context);

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	TObjectPtr<USkillDataAsset> SkillDataAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill Cooldown")
	TArray<int32> CurrentCooldown;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USkillComponent> OwnerComp;

	UPROPERTY()
	TObjectPtr<USkillPresentation> ActivePresentation;
		
protected:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
	void ApplyEffectToTargets(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context);

	virtual void ApplyEffectToTargets_Implementation(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context) 
	{
		// 기본 구현은 아무것도 하지 않음. 필요에 따라 서브클래스에서 오버라이드 가능.
	}


private:
	void HandlePresentationFinished();
};

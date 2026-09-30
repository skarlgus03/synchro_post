#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UnitAIBrain.generated.h"

class AUnit;


/*
 * AI 유닛에게 "플레이어의 손" 역할.
 * 턴이 오면 결정하고, 공용 입구(RequestMove / ExecuteSkill)로 명령하고, FinishTurn으로 끝낸다.
 *
 * - UnitDA의 레시피를 유닛마다 DuplicateObject해서 쓴다 (Outer = 유닛). 상태를 가져도 된다
 * - 서버 전용. 복제하지 않는다
 * - 세상에 영향을 주는 건 공용 입구와 FinishTurn뿐. 유닛·그리드 상태를 직접 고치지 않는다
 *
 * 베이스 구현은 아무것도 안 하고 턴을 넘긴다 (허수아비 / 테스트용).
 */ 
UCLASS(EditInlineNew, BlueprintType, Blueprintable)
class SYNCHROPOST_API UUnitAIBrain : public UObject
{
	 GENERATED_BODY()

 public:
	 virtual UWorld* GetWorld() const override;

	 /** 유닛의 턴 시작에서 호출. 한 틱 뒤 ExecuteTurn을 부른다 */
	 void BeginTurn();

 protected:
	 /** 결정 + 명령. 끝나면 반드시 FinishTurn을 부를 것 */
	 UFUNCTION(BlueprintNativeEvent, Category = "AI")
	 void ExecuteTurn();
	 virtual void ExecuteTurn_Implementation();

	 /** 턴 종료. 중복 호출·이미 남의 턴인 경우는 무시한다 */
	 UFUNCTION(BlueprintCallable, Category = "AI")
	 void FinishTurn();

	 UFUNCTION(BlueprintPure, Category = "AI")
	 AUnit* GetOwnerUnit() const;

 private:
	 void HandleDeferredExecute();
	 
	 bool bTurnActive = false;
};

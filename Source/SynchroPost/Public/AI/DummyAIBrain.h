#pragma once

#include "CoreMinimal.h"
#include "AI/UnitAIBrain.h"
#include "DummyAIBrain.generated.h"

class AUnit;

/*
* 더미 AI: 가장 가까운 적에게 가서 적을 맞출 수 있는 스킬을 AP가 바닥날 때까지 쓴다.
* 판단과 명령이 한 클래스에 있다. 
*/
UCLASS(EditInlineNew, BlueprintType)
class SYNCHROPOST_API UDummyAIBrain : public UUnitAIBrain
{
	GENERATED_BODY()
	
protected:
	virtual void ExecuteTurn_Implementation() override;

	/*한 턴 최대 행동 수. 비용 0 스킬로 인한 무한 루프 방지*/
	UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = 1))
	int32 MaxActionsPerTurn = 10;

private:
	bool TryUseBestSkill(AUnit* Self);
	bool TryMoveTowardNearestHostile(AUnit* Self);

	int32 CountHostilesInTiles(const AUnit* Self, const TArray<FIntPoint>& Tiles) const;

	static bool IsHostile(const AUnit* Self, const AUnit* Other);
	static int32 GridDistance(const FIntPoint& A, const FIntPoint& B)
	{
		return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
	}
};

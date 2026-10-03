#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UnitPresentationBase.generated.h"

class AUnit;

UCLASS()
class SYNCHROPOST_API UUnitPresentationBase : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Presentation")
	void PresentDeath(AUnit* Owner);
	virtual void PresentDeath_Implementation(AUnit* Owner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Presentation")
	void PresentRevive(AUnit* Owner);
	virtual void PresentRevive_Implementation(AUnit* Owner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Presentation")
	void PresentMoveSegment(AUnit* Owner, const TArray<FIntPoint>& Waypoints);
	virtual void PresentMoveSegment_Implementation(AUnit* Owner, const TArray<FIntPoint>& Waypoints);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Presentation")
	void PresentHit(AUnit* Owner);
	virtual void PresentHit_Implementation(AUnit* Owner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Presentation")
	void PresentFace(AUnit* Owner, const FIntPoint& From, const FIntPoint& Toward);
	virtual void PresentFace_Implementation(AUnit* Owner, const FIntPoint& From, const FIntPoint& Toward);

	virtual void TickPresentation(AUnit* Owner, float DeltaTime) {}
	virtual bool IsPresentingMove() const { return false; }
	virtual bool NeedsTick() const { return false; }
	virtual float GetPresentationMoveSpeed() const { return 0.f; }

protected:

	/*From->Toward 를 4방향 Yaw로 바꿈. 같은칸이면 false*/
	static bool CalcFacingRotation(const FIntPoint& From, const FIntPoint& Toward, FRotator& OutRotation);
};

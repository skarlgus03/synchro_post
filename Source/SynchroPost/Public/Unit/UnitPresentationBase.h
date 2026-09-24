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
	void PresentMoveSegment(AUnit* Owner, const FIntPoint& From, const FIntPoint& To);
	virtual void PresentMoveSegment_Implementation(AUnit* Owner, const FIntPoint& From, const FIntPoint& To);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Presentation")
	void PresentHit(AUnit* Owner);
	virtual void PresentHit_Implementation(AUnit* Owner);

	virtual void TickPresentation(AUnit* Owner, float DeltaTime) {}
	virtual bool IsPresentingMove() const { return false; }
	virtual bool NeedsTick() const { return false; }
	virtual float GetPresentationMoveSpeed() const { return 0.f; }
};

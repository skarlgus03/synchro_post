#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/SynchroPostTypes.h"
#include "TileTrigger.generated.h"


UINTERFACE(MinimalAPI)
class UTileTrigger : public UInterface
{
	GENERATED_BODY()
};

class SYNCHROPOST_API ITileTrigger
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	UFUNCTION(BlueprintNativeEvent, Category = "Trigger")
	FCombatEventTarget OnUnitEnter(class AUnit* Unit);

	UFUNCTION(BlueprintNativeEvent, Category = "Trigger")
	void PresentTriggerEffect(const FCombatEventTarget& Result);
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "StateComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API UStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UStateComponent();
	
protected:
	
	virtual void BeginPlay() override;

public:	

	const FGameplayTagContainer& GetStateTagContainer() const { return StateTags; }
	
protected:

	UPROPERTY()
	FGameplayTagContainer StateTags;

};

#pragma once

#include "CoreMinimal.h"
#include "Types/SynchroPostTypes.h"
#include "Components/ActorComponent.h"
#include "UnitSlotComponent.generated.h"

class UUnitSlot;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API UUnitSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUnitSlotComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	
	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	

	 TArray<UUnitSlot*>& GetUnitSlots()  { return UnitSlots; }

	 UUnitSlot* GetUnitSlotByIndex(int32 Index) ;

protected:

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_UnitSlots, VisibleAnywhere, BlueprintReadOnly, Category = "UnitSlot")
	TArray<UUnitSlot*> UnitSlots;

	UFUNCTION()
	void OnRep_UnitSlots();
};

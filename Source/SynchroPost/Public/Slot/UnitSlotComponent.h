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
	UUnitSlotComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


	
public:	
	
	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	

	// 슬롯이 초기화되어 있는지 확인하고, 초기화되지 않았다면 슬롯을 생성합니다.
	UFUNCTION(BlueprintCallable, Category = "UnitSlot")
	void EnsureSlotsInitialized();



	TArray<UUnitSlot*>& GetUnitSlots()  { return UnitSlots; }
	UUnitSlot* GetUnitSlotByIndex(int32 Index) ;

protected:

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_UnitSlots, VisibleAnywhere, BlueprintReadOnly, Category = "UnitSlot")
	TArray<UUnitSlot*> UnitSlots;

	UFUNCTION()
	void OnRep_UnitSlots();


	// 유닛 슬롯의 최대 슬롯수를 설정합니다. 기본값 6
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitSlot")
	int32 MaxSlots = 6;
};

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


	/* 이 유닛이 배정된 슬롯을 찾는다. 서버 클라 양쪽에서 동작함
	*/
	UUnitSlot* FindSlotOfUnit(const AUnit* Unit) const;

	// 이 유닛을 조종하는 플레이어. 없으면 nullptr
	UFUNCTION(BlueprintCallable, Category = "UnitSlot")
	APlayerState* GetOwnerOfUnit(const AUnit* Unit) const;

	// 서버 전용. 슬롯의 담당자를 바꿈
	void SetSlotOwner(int32 SlotIndex, APlayerState* NewOwner);

	// 서버 전용. 현재 접속한 플레이어들에게 슬롯을 분배함.
	void DistributeSlotsEvenly(const TArray<APlayerState*>& PlaeryStates);

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

#include "Slot/UnitSlotComponent.h"
#include "Slot/UnitSlot.h"
#include "Net/UnrealNetwork.h"

UUnitSlotComponent::UUnitSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	bReplicateUsingRegisteredSubObjectList = true;
}

// Called when the game starts
void UUnitSlotComponent::BeginPlay()
{
	Super::BeginPlay();
		

	EnsureSlotsInitialized();
}

void UUnitSlotComponent::EnsureSlotsInitialized()
{
	// 이미 만들어져 있으면 통과
	if (UnitSlots.Num() > 0) return;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		for (int32 i = 0; i < MaxSlots; ++i)
		{
			UUnitSlot* NewUnitSlot = NewObject<UUnitSlot>(this, UUnitSlot::StaticClass());
			if (NewUnitSlot)
			{
				AddReplicatedSubObject(NewUnitSlot);
				UnitSlots.Add(NewUnitSlot);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[UnitSlotComponent] EnsureSlotsInitialized: Spawn Slots for Owner: %s"), *GetOwner()->GetName());
	}
}

void UUnitSlotComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UUnitSlotComponent, UnitSlots);
}


UUnitSlot* UUnitSlotComponent::GetUnitSlotByIndex(int32 Index) 
{
	if (UnitSlots.IsValidIndex(Index))
	{
		return UnitSlots[Index];
	}
	return nullptr;
}

 void UUnitSlotComponent::OnRep_UnitSlots()
 {
	 UE_LOG(LogTemp, Log, TEXT("[UnitSlotComponent] OnRep_UnitSlots Called! UnitSlots Count: %d"), UnitSlots.Num());

	 for (UUnitSlot* UnitSlot : UnitSlots)
	 {
		 if (UnitSlot)
		 {
			 UE_LOG(LogTemp, Log, TEXT("[UnitSlotComponent] UnitSlot: %s"), *UnitSlot->GetName());
		 }
		 else
		 {
			 UE_LOG(LogTemp, Warning, TEXT("[UnitSlotComponent] Null UnitSlot in UnitSlots array"));
		 }
	 }
	 
 }






#include "Slot/UnitSlotComponent.h"
#include "Slot/UnitSlot.h"
#include "Net/UnrealNetwork.h"

UUnitSlotComponent::UUnitSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);

	bReplicateUsingRegisteredSubObjectList = true;
}

// Called every frame
void UUnitSlotComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// Called when the game starts
void UUnitSlotComponent::BeginPlay()
{
	Super::BeginPlay();
		

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		AActor* OwnerActor = GetOwner();
		if (OwnerActor)
		{
			for (int32 i = 0; i < 6; ++i)
			{
				UUnitSlot* NewUnitSlot = NewObject<UUnitSlot>(this, UUnitSlot::StaticClass());
				if (NewUnitSlot)
				{
					AddReplicatedSubObject(NewUnitSlot);

					UnitSlots.Add(NewUnitSlot);
				}
			}

			UE_LOG(LogTemp, Log, TEXT("[UnitSlotComponent] Spawn Slots for Owner: %s"), *GetOwner()->GetName());
		}
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






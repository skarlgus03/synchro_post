#include "Slot/UnitSlotComponent.h"
#include "Slot/UnitSlot.h"
#include "Net/UnrealNetwork.h"
#include "SynchroPost.h"
#include "Framework/SPPlayerState.h"

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


 UUnitSlot* UUnitSlotComponent::FindSlotOfUnit(const AUnit* Unit) const
 {
	 if (!Unit)
	 {
		 return nullptr;
	 }

	 for (UUnitSlot* Slot : UnitSlots)
	 {
		 if (Slot && Slot->GetCurrentUnit() == Unit)
		 {
			 return Slot;
		 }
	 }
	 return nullptr;
 }

 APlayerState* UUnitSlotComponent::GetOwnerOfUnit(const AUnit* Unit) const
 {
	 const UUnitSlot* Slot = FindSlotOfUnit(Unit);
	 return Slot ? Slot->GetOwnerPlayerState() : nullptr;
 }

 void UUnitSlotComponent::SetSlotOwner(int32 SlotIndex, APlayerState* NewOwner)
 {
	 if (!GetOwner() || !GetOwner()->HasAuthority())
	 {
		 UE_LOG(LogSP, Warning, TEXT("[Slot] SetSlotOwner는 서버 전용이다. (Index=%d)"), SlotIndex);
		 return;
	 }

	 if (!UnitSlots.IsValidIndex(SlotIndex) || !UnitSlots[SlotIndex])
	 {
		 UE_LOG(LogSP, Warning, TEXT("[Slot] SetSlotOwner: 유효하지 않은 슬롯 %d"), SlotIndex);
		 return;
	 }

	 UnitSlots[SlotIndex]->SetOwnerPlayerState(NewOwner);

	 UE_LOG(LogSP, Log, TEXT("[Slot] %d번 슬롯 담당 → Id=%d (%s)"),
		 SlotIndex,
		 NewOwner ? NewOwner->GetPlayerId() : -1,
		 NewOwner ? *NewOwner->GetPlayerName() : TEXT("없음"));
 }

 void UUnitSlotComponent::DistributeSlotsEvenly(const TArray<APlayerState*>& Players)
 {
	 if (!GetOwner() || !GetOwner()->HasAuthority())
	 {
		 return;
	 }

	 EnsureSlotsInitialized();

	 if (Players.Num() == 0)
	 {
		 for (int32 i = 0; i < UnitSlots.Num(); ++i)
		 {
			 SetSlotOwner(i, nullptr);
		 }
		 return;
	 }

	 for (int32 i = 0; i < UnitSlots.Num(); ++i)
	 {
		 SetSlotOwner(i, Players[i % Players.Num()]);
	 }
 }


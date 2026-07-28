#include "Unit/GridMoveComponent.h"
#include "Net/UnrealNetwork.h"
#include "Unit/Unit.h"
#include "Unit/StateComponent.h"
#include "Types/SPCombatEventStructure.h"
#include "Framework/CombatEventComponent.h"
#include "Framework/GridManager.h"


// Sets default values for this component's properties
UGridMoveComponent::UGridMoveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

}


// Called when the game starts
void UGridMoveComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerUnit = Cast<AUnit>(GetOwner());
	if (UWorld* World = GetWorld())
	{
		CachedGridManager = World->GetSubsystem<UGridManager>();
	}
}

void UGridMoveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGridMoveComponent, CurrentMovePoint);
}

bool UGridMoveComponent::RequestMove(const TArray<FIntPoint>& Path)
{
	if (!OwnerUnit || !CachedGridManager || Path.Num() == 0)
	{
		return false;
	}

	if (!CanMove())
	{
		return false;
	}

	const int32 Cost = Path.Num();
	if (GetAvailableMovePoint() < Cost)
	{
		return false;
	}

	TArray<FMoveStep> Steps = CachedGridManager->MoveUnitAlongPath(OwnerUnit, Path);
	CurrentMovePoint -= Cost;

	UCombatEventComponent* EventComp = OwnerUnit->GetCombatEventComponent();
	if (!EventComp)
	{
		return true;
	}

	for (const FMoveStep& Step : Steps)
	{
		FCombatEvent Event;
		Event.Source = OwnerUnit;

		if (Step.StepType == EMoveStepType::Segment)
		{
			FMoveEventPayload SegmentPayload;
			SegmentPayload.From = Step.From;
			SegmentPayload.To = Step.To;
			Event.Payload = FInstancedStruct::Make(SegmentPayload);
		}
		else
		{
			FTriggerEventPayload TriggerPayload;
			TriggerPayload.Trigger = Step.Trigger;
			TriggerPayload.Result = Step.Result;
			Event.Payload = FInstancedStruct::Make(TriggerPayload);
		}
		EventComp->PushEvent(Event);
	}

	return true;
}

bool UGridMoveComponent::CanMove() const
{
	return GetAvailableMovePoint() > 0;
}

float UGridMoveComponent::GetMovementPenaltyMultiplier() const
{
	if (!OwnerUnit || !OwnerUnit->GetStateComponent())
	{
		return 1.0f;
	}
	
	float WorstPenalty = 0.0f;
	const FGameplayTagContainer& CurrentTags = OwnerUnit->GetStateComponent()->GetStateTags();
	for (const auto& Pair : MovementPenaltyTable)
	{
		if (CurrentTags.HasTag(Pair.Key))
		{
			WorstPenalty = FMath::Max(WorstPenalty, Pair.Value);
		}
	}
	return 1.0f - FMath::Clamp(WorstPenalty, 0.0f, 1.0f);
}

void UGridMoveComponent::RefillMovePoint()
{
	if (OwnerUnit)
	{
		BaseMovePoint = FMath::FloorToInt32(OwnerUnit->GetSpeed() * SpeedToMovePointRatio);
		CurrentMovePoint = BaseMovePoint;
	}
}

int32 UGridMoveComponent::GetAvailableMovePoint() const
{
	const int32 PenalizedCap = FMath::FloorToInt32(BaseMovePoint * GetMovementPenaltyMultiplier());	

	return FMath::Min(CurrentMovePoint, PenalizedCap);
}

void UGridMoveComponent::OnRep_MovePoint()
{
}





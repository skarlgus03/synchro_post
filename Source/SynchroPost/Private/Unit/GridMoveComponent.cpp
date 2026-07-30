#include "Unit/GridMoveComponent.h"
#include "Net/UnrealNetwork.h"
#include "Unit/Unit.h"
#include "Math/StatMath.h"
#include "Unit/StateComponent.h"
#include "Types/SPCombatEventStructure.h"
#include "Framework/CombatEventComponent.h"
#include "Framework/SynchroPostSettings.h"
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
	DOREPLIFETIME(UGridMoveComponent, BaseMovePoint);
}

bool UGridMoveComponent::RequestMove(const TArray<FIntPoint>& Path)
{
	if (!OwnerUnit || !CachedGridManager || Path.Num() == 0)
	{
		return false;
	}

	const int32 Cost = Path.Num();

	if (GetAvailableMovePoint() < Cost)
	{
		return false;
	}

	// 이동을 수행하고, 이동 경로에 따라 발생하는 이벤트를 받아온다.
	TArray<FMoveStep> Steps = CachedGridManager->MoveUnitAlongPath(OwnerUnit, Path);

	CurrentMovePoint -= Cost;

	
	UCombatEventComponent* EventComp = OwnerUnit->GetCombatEventComponent();
	if (!EventComp)
	{
		return true;
	}

	// FMoveStep 을 확인하며, 이동 이벤트/트리거이벤트를 발생시킨다.
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

	const USynchroPostSettings* GlobalRules = GetDefault<USynchroPostSettings>();
	const FGameplayTagContainer CurrentTags = OwnerUnit->GetStateComponent()->GetStateTags();

	int32 WorstRemainingPercent = 100; // 기본값: 페널티 없음

	for (const FGameplayTag& Tag : CurrentTags)
	{
		if (const int32* Override = PenaltyOverrides.Find(Tag))
		{
			WorstRemainingPercent = FMath::Min(WorstRemainingPercent, *Override);
		}
		else if (const int32* GlobalValue = GlobalRules->DefaultMovementPenaltyTable.Find(Tag))
		{
			WorstRemainingPercent = FMath::Min(WorstRemainingPercent, *GlobalValue);
		}
	}

	return FMath::Clamp(StatMath::PercentToFloat(WorstRemainingPercent), 0.0f, 1.0f);
}

void UGridMoveComponent::RefillMovePoint()
{
	if (OwnerUnit)
	{
		const USynchroPostSettings* GlobalRules = GetDefault<USynchroPostSettings>();
		BaseMovePoint = FMath::FloorToInt32(OwnerUnit->GetSpeed() * GlobalRules->SpeedToMovePointRatio);
		CurrentMovePoint = BaseMovePoint;
		UE_LOG(LogTemp, Log, TEXT("RefillMovePoint: %s, BaseMovePoint=%d, CurrentMovePoint=%d"), *OwnerUnit->GetName(), BaseMovePoint, CurrentMovePoint);
	}
}

void UGridMoveComponent::SetMovementPenaltyOverride(const FGameplayTag& Tag, float NewPenalty)
{
	PenaltyOverrides.Add(Tag, NewPenalty);
}

int32 UGridMoveComponent::GetAvailableMovePoint() const
{
	const int32 PenalizedCap = FMath::FloorToInt32(BaseMovePoint * GetMovementPenaltyMultiplier());
	return FMath::Min(CurrentMovePoint, PenalizedCap);
}

void UGridMoveComponent::OnRep_MovePoint()
{
}





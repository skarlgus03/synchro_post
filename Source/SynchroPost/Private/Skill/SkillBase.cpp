#include "Skill/SkillBase.h"
#include "Unit/Unit.h"
#include "Skill/SkillDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Unit/SkillComponent.h"
#include "Framework/CombatEventComponent.h"
#include "Framework/GridManager.h"


int32 USkillBase::GetCurrentCooldown(const FGameplayTagContainer& StatusTags) const
{
	const int32 CurrentStateIndex = DetermineCurrentIndex(StatusTags);

	check(CurrentCooldown.IsValidIndex(CurrentStateIndex));

	return CurrentCooldown[CurrentStateIndex];
}

const FSkillData& USkillBase::GetSkillDataByIndex(int32 Index) const
{
	check(SkillDataAsset->SkillDataArray.IsValidIndex(Index));

	return SkillDataAsset->SkillDataArray[Index];
}

bool USkillBase::IsWithinCastRange(const FIntPoint& TargetCoord, const int32& CastRange, const FSkillExecutionContext& Context) const
{
	
	FIntPoint CasterCoord = Context.CasterCoordinate;

	int32 Distance = FMath::Abs(TargetCoord.X - CasterCoord.X) + FMath::Abs(TargetCoord.Y - CasterCoord.Y);
	return Distance <= CastRange;

}

bool USkillBase::MatchesFaction(ESkillTargetFaction SkillTargetFaction, const FSkillExecutionContext& Context, EFaction TargetUnitFaction) const
{
	
	if (SkillTargetFaction == ESkillTargetFaction::Any)
	{
		return true;
	}

	bool bIsSameFaction = (TargetUnitFaction == Context.CasterFaction);

	if (SkillTargetFaction == ESkillTargetFaction::Ally)
	{
		return bIsSameFaction;
	}
	if (SkillTargetFaction == ESkillTargetFaction::Enemy)
	{
		return !bIsSameFaction;
	}

	return false;
}


void USkillBase::InitializeSkill(USkillDataAsset* InSkillDataAsset)
{
	if (!InSkillDataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("USkillBase::InitializeSkill - InSkillDataAsset is null."));
		return;
	}

	SkillDataAsset = InSkillDataAsset;

	CurrentCooldown.SetNum(SkillDataAsset->SkillDataArray.Num());
	for (int32& Cooldown : CurrentCooldown)
	{
		Cooldown = 0;
	}

}

void USkillBase::PushSkillCombatEvent(const FSkillExecutionContext& Context, const TArray<FCombatEventTarget>& Targets) 
{
	AUnit* Caster = OwnerComp ? OwnerComp->GetOwnerUnit() : nullptr;

	if (!Caster)
	{
		return;
	}

	UCombatEventComponent* EventComp = Caster->GetCombatEventComponent();
	if (!EventComp)
	{
		return;
	}

	FSkillEventPayload SkillPayload;
	SkillPayload.SkillTag = Context.SkillSlotTag;
	SkillPayload.Targets = Targets;

	FCombatEvent Event;
	Event.Source = Caster;
	Event.Payload = FInstancedStruct::Make(SkillPayload);

	EventComp->PushEvent(Event);
}

void USkillBase::NotifySkillEffectPresentationFinished() const
{
	if (AUnit* Caster = OwnerComp ? Cast<AUnit>(OwnerComp->GetOwner()) : nullptr)
	{
		if (UCombatEventComponent* EventComp = Caster->GetCombatEventComponent())
		{
			EventComp->NotifyPresentationFinished();
		}
	}
}

void USkillBase::ExecuteSkill_Implementation(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context)
{
	const int32 Index = DetermineCurrentIndex(Context.StateTags);
	const FSkillData& Data = GetCurrentSkillData(Context.StateTags);

	if (CurrentCooldown.IsValidIndex(Index))
	{
		CurrentCooldown[Index] = Data.BaseCooldown;
	}

	ApplyEffectToTargets(TargetData, Context);
}

bool USkillBase::CanExecuteOnTarget_Implementation(const FSkillTargetData& TargetData, const FSkillExecutionContext& Context) const
{
	const FSkillTargetingRule& Rule = GetTargetingRule(Context.StateTags);

	if (TargetData.SelectedTiles.Num() != Rule.RequiredTileSelectionCount)
	{
		return false;
	}

	// 월드 서브시스템 그리드매니저를 가져온다.
	UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();
	if (!GridManager)
	{
		return false;
	}

	for (const FIntPoint& Coord : TargetData.SelectedTiles)
	{

		// 사거리 검증
		if (!IsWithinCastRange(Coord, Rule.CastRange, Context))
		{
			return false;
		}

		// 진영 검증
		if (Rule.TargetFaction != ESkillTargetFaction::None)
		{
			AUnit* TargetUnit = GridManager->GetUnitAt(Coord);
			if (!TargetUnit || !MatchesFaction(Rule.TargetFaction, Context, TargetUnit->GetFaction()))
			{
				return false;
			}
		}
	}

	return true;
}


const FSkillData& USkillBase::GetCurrentSkillData(const FGameplayTagContainer& StateTags) const
{
	
	check(SkillDataAsset);

	const int32 Index = DetermineCurrentIndex(StateTags);

	check(SkillDataAsset->SkillDataArray.IsValidIndex(Index));

	return SkillDataAsset->SkillDataArray[Index];
}

const FSkillTargetingRule& USkillBase::GetTargetingRule(const FGameplayTagContainer& StateTags) const
{
	return GetCurrentSkillData(StateTags).TargetingRule;
}


TArray<FIntPoint> USkillBase::GetAffectedTiles_Implementation(const FIntPoint& TargetCoord, const FSkillExecutionContext& Context) const
{
	const FSkillTargetingRule& Rule = GetTargetingRule(Context.StateTags);

	if (Rule.RangePatternOffsets.Num() == 0)
	{
		return { TargetCoord };
	}

	TArray<FIntPoint> AffectedTiles;
	for (const FIntPoint& Offset : Rule.RangePatternOffsets)
	{
		FIntPoint AffectedCoord = TargetCoord + Offset;
		AffectedTiles.Add(AffectedCoord);
	}
	return AffectedTiles;
}

TArray<FIntPoint> USkillBase::GetRangeTiles(const FSkillExecutionContext& Context) const
{
	TArray<FIntPoint> RangeTiles;

	UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();

	if (!GridManager)
	{
		return RangeTiles;
	}

	const FSkillTargetingRule& Rule = GetTargetingRule(Context.StateTags);

	for (const FTile& Tile : GridManager->GetAllTiles())
	{
		if (IsWithinCastRange(Tile.Coordinate, Rule.CastRange, Context))
		{
			RangeTiles.Add(Tile.Coordinate);
		}
	}

	return RangeTiles;
}

TArray<FIntPoint> USkillBase::GetValidTargetTiles(const FSkillExecutionContext& Context) const
{
	TArray<FIntPoint> ValidTiles;

	UGridManager* GridManager = GetWorld()->GetSubsystem<UGridManager>();

	if (!GridManager)
	{
		return ValidTiles;
	}
	for (const FTile& Tile : GridManager->GetAllTiles())
	{
		FSkillTargetData CandidateData;
		CandidateData.SelectedTiles.Add(Tile.Coordinate);

		if (CanExecuteOnTarget(CandidateData, Context))
		{
			ValidTiles.Add(Tile.Coordinate);
		}
	}

	return ValidTiles;
}

void USkillBase::DecreaseCooldowns()
{
	for (int32& Cooldown : CurrentCooldown)
	{
		if (Cooldown > 0)
		{
			Cooldown--;
		}
	}
}

void USkillBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USkillBase, CurrentCooldown);
}

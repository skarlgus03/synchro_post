#include "Skill/SkillBase.h"
#include "Unit/Unit.h"
#include "Skill/SkillDataAsset.h"
#include "Net/UnrealNetwork.h"
#include "Unit/SkillComponent.h"
#include "FrameWork/GridManager.h"


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

bool USkillBase::MatchesFaction(AUnit* Target, ESkillTargetFaction TargetFaction, const FSkillExecutionContext& Context) const
{
	if (!Target)
	{
		return false;
	}

	if (TargetFaction == ESkillTargetFaction::Any)
	{
		return true;
	}
		

	bool bIsSameFaction = (Context.CasterFaction == Target->GetFaction());

	if (TargetFaction == ESkillTargetFaction::Ally)
	{
		return bIsSameFaction;
	}
	if (TargetFaction == ESkillTargetFaction::Enemy)
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

void USkillBase::ExecuteSkill_Implementation(AUnit* Caster, const TArray<AUnit*>& Target)
{
	
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

	for (const FIntPoint& Coord : TargetData.SelectedTiles)
	{

		if (!IsWithinCastRange(Coord, Rule.CastRange, Context))
		{
			return false;
		}

		if (Rule.TargetFaction != ESkillTargetFaction::None)
		{
			AUnit* Unit = GridManager->GetUnitAt(Coord);
			if (!Unit || !MatchesFaction(Unit, Rule.TargetFaction, Context))
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

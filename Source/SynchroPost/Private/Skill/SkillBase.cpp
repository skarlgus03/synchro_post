#include "Skill/SkillBase.h"
#include "Unit/Unit.h"

void USkillBase::InitializeSkill()
{
	CurrentCooldown.Init(0, SkillDataArray.Num());
}

void USkillBase::ExecuteSkill_Implementation(AUnit* Caster, const TArray<AUnit*>& Target)
{
	if (!CanExecuteSkill(Caster, Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot execute skill %s. Cooldown: %d"), *GetName(), CurrentCooldown[CurrentStateIndex]);
		return;
	}
	const FSkillData& SkillData = GetCurrentSkillData(Caster);
	if (CurrentCooldown.IsValidIndex(CurrentStateIndex))
	{
		CurrentCooldown[CurrentStateIndex] = SkillData.BaseCooldown;
	}
}

void USkillBase::UpdateCurrentIndex(AUnit* Caster)
{
	CurrentStateIndex = DetermineCurrentIndex(Caster);

	if (!SkillDataArray.IsValidIndex(CurrentStateIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid skill data index %d for skill %s. Resetting to 0."), CurrentStateIndex, *GetName());
		CurrentStateIndex = 0;
	}
}

const FSkillData& USkillBase::GetCurrentSkillData(AUnit* Caster) const
{
	
	if (SkillDataArray.IsValidIndex(CurrentStateIndex))
	{
		return SkillDataArray[CurrentStateIndex];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid skill data index %d for skill %s. Returning first skill data as fallback."), CurrentStateIndex, *GetName());
		return SkillDataArray[0];
	}
}

bool USkillBase::CanExecuteSkill(AUnit* Caster, const TArray<AUnit*>& Target) const
{
	
	if (CurrentCooldown.IsValidIndex(CurrentStateIndex) && CurrentCooldown[CurrentStateIndex] > 0)
	{
		return false;
	}
	if (SkillDataArray[CurrentStateIndex].APCost > Caster->ActionPoints)
	{
		return false;
	}
	if (SkillDataArray[CurrentStateIndex].BPCost > Caster->BehaviorPoints)
	{
		return false;
	}

	return CanExecuteSkill_BP(Caster, Target);
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

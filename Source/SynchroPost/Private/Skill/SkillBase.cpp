#include "Skill/SkillBase.h"
#include "Unit/Unit.h"
#include "Skill/SkillDataAsset.h"
#include "Net/UnrealNetwork.h"

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


const FSkillData& USkillBase::GetCurrentSkillData(const FGameplayTagContainer& StatusTags) const
{
	
	check(SkillDataAsset);

	const int32 Index = DetermineCurrentIndex(StatusTags);

	check(SkillDataAsset->SkillDataArray.IsValidIndex(Index));

	return SkillDataAsset->SkillDataArray[Index];
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

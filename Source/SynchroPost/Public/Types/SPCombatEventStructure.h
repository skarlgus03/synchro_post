#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/SynchroPostTypes.h"
#include "SPCombatEventStructure.generated.h"

class AUnit;

UENUM(BlueprintType)
enum class ECombatEventType : uint8
{
	SkillUsed,
	UnitDied,
	UnitRevived,
};

USTRUCT(BlueprintType)
struct FCombatEventTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Combat Event")
	TWeakObjectPtr<AUnit> Target;

	UPROPERTY(BlueprintReadWrite, Category = "Combat Event")
	FSPDamageData DamageData;
};

USTRUCT(BlueprintType)
struct FCombatEvent 
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	ECombatEventType EventType = ECombatEventType::SkillUsed;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TWeakObjectPtr<AUnit> Source;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FGameplayTag SkillTag;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TArray<FCombatEventTarget> Targets;

};

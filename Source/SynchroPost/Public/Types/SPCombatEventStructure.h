#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/SynchroPostTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Interface/TileTrigger.h"
#include "SPCombatEventStructure.generated.h"

class AUnit;


UENUM(BlueprintType)
enum class EMoveStepType : uint8
{
	Segment,
	Trigger
};

USTRUCT(BlueprintType)
struct FMoveStep
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	EMoveStepType StepType = EMoveStepType::Segment;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FIntPoint From;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FIntPoint To;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TScriptInterface<ITileTrigger> Trigger;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FCombatEventTarget Result;
};


USTRUCT(BlueprintType)
struct FSkillEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FGameplayTag SkillTag;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TArray<FCombatEventTarget> Targets;
};

USTRUCT(BlueprintType)
struct FUnitDiedPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TWeakObjectPtr<AUnit> Causer;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FIntPoint DeathCoordinate;
};

USTRUCT(BlueprintType)
struct FUnitRevivedPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TWeakObjectPtr<AUnit> Causer;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FIntPoint RevivalCoordinate;
};

USTRUCT(BlueprintType)
struct FItemUsePayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FGameplayTag ItemTag;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TArray<FCombatEventTarget> Targets;
};

USTRUCT(BlueprintType)
struct FMoveEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FIntPoint From;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FIntPoint To;
};

USTRUCT(BlueprintType)
struct FTriggerEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TScriptInterface<ITileTrigger> Trigger;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FCombatEventTarget Result;
};

USTRUCT(BlueprintType)
struct FCombatEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TWeakObjectPtr<AUnit> Source;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FInstancedStruct Payload;
};

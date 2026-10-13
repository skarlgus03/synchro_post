#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/SynchroPostTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Interface/TileTrigger.h"
#include "Types/SPSkillStructure.h"
#include "SPCombatEventStructure.generated.h"

class AUnit;
class USkillDataAsset;

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
	FIntPoint From = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FIntPoint To = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TArray<FIntPoint> Waypoints; // 시작 칸 포함, 끝 칸 포함

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
	FSkillExecutionContext Context;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TArray<FCombatEventTarget> Targets;

	// 실제로 나간 스킬. 연출은 슬롯으로 다시 고르지 않고 이걸로 인스턴스를 찾는다
	// (연출 시점엔 폼·스택이 바뀌어 슬롯의 스킬이 달라져 있을 수 있다)
	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TObjectPtr<USkillDataAsset> SkillAsset;
};

USTRUCT(BlueprintType)
struct FUnitDeathEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TWeakObjectPtr<AUnit> Unit;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TWeakObjectPtr<AActor> Causer;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FIntPoint DeathCoordinate = FIntPoint::ZeroValue;
};


USTRUCT(BlueprintType)
struct FUnitsDiedPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TArray<FUnitDeathEntry> Deaths;
};

USTRUCT(BlueprintType)
struct FUnitRevivedPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	TWeakObjectPtr<AActor> Causer;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Event")
	FIntPoint RevivalCoordinate = FIntPoint::ZeroValue;
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
	TArray<FIntPoint> Waypoints; // 시작 칸 포함, 끝 칸 포함

	FIntPoint GetFrom() const { return Waypoints.Num() > 0 ? Waypoints[0] : FIntPoint::ZeroValue; }
	FIntPoint GetTo() const { return Waypoints.Num() > 0 ? Waypoints.Last() : FIntPoint::ZeroValue; }
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

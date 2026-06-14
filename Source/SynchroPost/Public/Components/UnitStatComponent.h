// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/SynchroPostTypes.h"
#include "Types/SPGameplayTags.h"
#include "UnitStatComponent.generated.h"

class UUnitStatDataAsset;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API UUnitStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUnitStatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


public:

	// 스탯 맵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Unit Stat")
	TMap<FGameplayTag, FStatDetailed> StatMap;

	// 현재 적용중인 스탯 데이터 에셋
	UPROPERTY(BlueprintReadOnly, Category = "Unit Stat")
	TObjectPtr<const UUnitStatDataAsset> CurrentStatData;

public:

	// 스탯 초기화
	void InitializeStats(const UUnitStatDataAsset* StatData, int32 Level);



	// 스탯 수정자 업데이트
	void ApplyModifiers(FGameplayTag SourceTag, const TArray<FStatModifier>& Modifiers);

	// 스탯 수정자 제거
	void RemoveModifiers(FGameplayTag SourceTag);


	// 모든 스탯 업데이트
	void RefreshAllStats();

	UFUNCTION(BlueprintCallable, Category = "Unit Stat")
	int32 GetStat(FGameplayTag StatTag);

protected:

	// 유닛 스탯 데이터로 초기화하는 헬퍼 함수
	void InitializeStatsFromUnitStatData(const UUnitStatDataAsset* StatData);

	// 슬롯 레벨로 초기화하는 헬퍼 함수
	void InitializeStatsFromLevel(int32 Level);

	// 기본 값으로 초기화하는 헬퍼 함수
	void InitializeStatsToGlobalBaseValue();
};

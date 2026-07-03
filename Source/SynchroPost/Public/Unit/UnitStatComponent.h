#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/SynchroPostTypes.h"
#include "Types/SPGameplayTags.h"
#include "UnitStatComponent.generated.h"

class UUnitStatDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChangedSignature, int32, NewHealth, int32 , DamageAmount,const FGameplayTagContainer& , DamageTypeTagContainer);

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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:

	UPROPERTY(BlueprintAssignable, Category = "Unit Stat")
	FOnHealthChangedSignature OnHealthChanged;

public:

	// 스탯 초기화
	void InitializeStats(const UUnitStatDataAsset* StatData, int32 Level);

	// 모든 스탯 업데이트
	void RefreshAllStats();

	// 데미지 적용 함수 예정
	int32 ApplyDamage(const FSPDamageData& DamageData);


	UFUNCTION(BlueprintCallable, Category = "Unit Stat")
	int32 GetStat(FGameplayTag StatTag);

	UFUNCTION(BlueprintCallable, Category = "Unit Stat")
	int32 GetResistance(FGameplayTag StatTag);


	UFUNCTION(BlueprintCallable, Category = "Unit Stat")
	int32 GetCurrentHealth() const { return CurrentHealth; }

protected:

	// 유닛 스탯 데이터로 초기화하는 헬퍼 함수
	void InitializeStatsFromUnitStatData(const UUnitStatDataAsset* StatData);

	// 슬롯 레벨로 초기화하는 헬퍼 함수
	void InitializeStatsFromLevel(int32 Level);

	void InitializeResistancesFromUnitStatData(const UUnitStatDataAsset* StatData);

	// 기본 값으로 초기화하는 헬퍼 함수
	void InitializeStatsToGlobalBaseValue();


	// 데미지에서 방어력을 계산해서 깎는 헬퍼 함수
	void CalculateDamageAfterDefense(FSPDamageData& DamageData);

	// 데미지에서 내성을 계산해서 깎는 헬퍼 함수
	void CalculateDamageAfterResistance(FSPDamageData& DamageData);


	void UpdateCachedStatModifier();

protected:


	// 스탯 맵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Stat")
	TMap<FGameplayTag, FUnitStat> StatMap;

	// 저항 수치 (없으면 0으로 간주)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Stat")
	TMap<FGameplayTag, int32> Resistances;



	// 현재 적용중인 스탯 데이터 에셋
	UPROPERTY(BlueprintReadOnly, Category = "Unit Stat")
	TObjectPtr<const UUnitStatDataAsset> CurrentStatData;


	TObjectPtr<class AUnit> OwnerUnit;


	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Unit Stat")
	int32 CurrentHealth = 0;
};

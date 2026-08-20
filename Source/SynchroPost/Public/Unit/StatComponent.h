#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/SynchroPostTypes.h"
#include "Types/SPGameplayTags.h"
#include "StatComponent.generated.h"

class UUnitStatDataAsset;
class UUnitDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, int32, NewHealth, const FSPDamageData&, DamageData);



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNCHROPOST_API UStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStatComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	UPROPERTY(BlueprintAssignable, Category = "Unit Stat")
	FOnHealthChanged OnHealthChanged;

public:

	// 스탯 초기화
	void InitializeStats(const UUnitStatDataAsset* StatData, int32 Level);

	// 기믹 스탯을 유닛 데이터로 초기화하는 헬퍼 함수
	void InitializeGimmickStats(const UUnitDataAsset* UnitData);


	// 모든 스탯 업데이트
	void RefreshAllStats();

	// 데미지 적용 함수 예정
	int32 ApplyDamage(const FSPDamageData& DamageData);

	// 장비 모디파이어를 받아 저장합니다.
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void SetSlotModifiers(const TArray<FStatModifierEntry>& NewModifiers);

	// 상태이상 모디파이어를 받아 저장합니다.
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void AddStatusEffectModifiers(const TArray<FStatModifierEntry>& NewModifiers);

	// this를 소스로 가진 상태이상 모디파이어를 제거합니다.
	UFUNCTION(BlueprintCallable, Category = "Stat")
	void RemoveStatusEffectModifiers(UObject* Source);

	UFUNCTION(BlueprintCallable, Category = "Gimmick Stat")
	void ModifyGimmickStat(FGameplayTag StatTag, int32 Delta);


	UFUNCTION(BlueprintCallable, Category = "Unit Stat")
	int32 GetStat(FGameplayTag StatTag);

	UFUNCTION(BlueprintCallable, Category = "Unit Stat")
	int32 GetResistance(FGameplayTag StatTag);

	UFUNCTION(BlueprintCallable, Category = "Unit Stat")
	int32 GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category = "Gimmick Stat")
	int32 GetGimmickStat(FGameplayTag StatTag) const;


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

	// 기믹 스탯 맵
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit Stat")
	TMap<FGameplayTag, int32> GimmickStatMap;

	// ===== 캐싱용 스탯 모디파이어 배열 =====

	// UnitSlot에서 받아온 FStatModifierEntry 배열 캐싱용
	UPROPERTY()
	TArray<FStatModifierEntry> SlotStatModifiers;

	// 상태이상으로 인한 FStatModifierEntry 배열 캐싱용
	UPROPERTY()
	TArray<FStatModifierEntry> StatusEffectModifiers;


	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Unit Stat")
	int32 CurrentHealth = 0;




	// 현재 적용중인 스탯 데이터 에셋
	UPROPERTY(BlueprintReadOnly, Category = "Unit Stat")
	TObjectPtr<const UUnitStatDataAsset> CurrentStatData;

	TObjectPtr<class AUnit> OwnerUnit;

};

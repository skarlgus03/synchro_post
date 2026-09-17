// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Types/SPGameplayTags.h"
#include "Types/SynchroPostTypes.h"
#include "Types/SPSkillStructure.h"
#include "Interface/Damageable.h"
#include "Unit.generated.h"

class UUnitDataAsset;
class UUnitSlot;
class USkillComponent;
class UStatComponent;
class UStateComponent;
class UGridMoveComponent;
class UCombatEventComponent;
class UUnitPresentationBase;
class UWidgetComponent;
class UUnitHealthBarWidget;
class UUnitAnimSetDataAsset;

/*체력 바 표시 이유 */
enum class EHealthBarReason : uint8
{
	None = 0,
	Damaged = 1 << 0,
	RecentHit = 1 << 1,
	Hovered = 1 << 2,
	Selected = 1 << 3,
};
ENUM_CLASS_FLAGS(EHealthBarReason)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDied, AUnit*, DeadUnit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitRevived, AUnit*, RevivedUnit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFactionChanged, AUnit*, Unit);

UCLASS()
class SYNCHROPOST_API AUnit : public ACharacter, public IDamageable
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AUnit();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:

	// 에디터에서 기본적으로 사용할 DA를 지정할 수 있는 프로퍼티
	UPROPERTY(EditAnywhere, Category = "Unit Data")
	TObjectPtr<UUnitDataAsset> DefaultUnitData;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UUnitSlot> CurrentSlot;

	// == 컴포넌트들 ==

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USkillComponent> SkillComponent;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UStatComponent> StatComponent;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UStateComponent> StateComponent;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UGridMoveComponent> GridMoveComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarWidgetComponent;



	UPROPERTY(ReplicatedUsing = OnRep_Faction, EditAnywhere, BlueprintReadOnly, Category = "Unit")
	EFaction Faction = EFaction::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Unit")
	bool bIsDead = false;

	// Current grid position of the unit on the map
	UPROPERTY(ReplicatedUsing = OnRep_GridPosition, EditAnywhere, Category = "Unit State|Grid")
	FIntPoint GridPosition;

	UPROPERTY(ReplicatedUsing = OnRep_UnitData)
	TSoftObjectPtr<UUnitDataAsset> ReplicatedUnitData;

	UPROPERTY()
	TObjectPtr<UUnitPresentationBase> PresentationBehavior;

	
public:

	// 게임 도중에 실시간으로 바뀔 수 있는 DA 프로퍼티
	UPROPERTY(BlueprintReadOnly, Category = "Unit Data")
	TObjectPtr<const UUnitDataAsset> CurrentUnitData;
	
	
	FOnUnitDied OnUnitDied;

	FOnUnitRevived OnUnitRevived;

	UPROPERTY(BlueprintAssignable, Category = "Unit")
	FOnFactionChanged OnFactionChanged;

public:

	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable)
	void InitializeUnit(const UUnitDataAsset* UnitData);

	// == Handle Functions ==

	UFUNCTION()
	void HandleHealthChanged(int32 NewHealth, const FSPHealthActionData& ActionData);

	UFUNCTION()
	void HandleTurnStart(AUnit* Unit);

	UFUNCTION()
	void HandleTurnEnd(AUnit* Unit);



	// Damageable 인터페이스 구현

	// 유닛의 체력 변화를 적용한다. 체력 변화량을 반환한다. (음수면 피해, 양수면 회복)
	int32 ApplyHealthChange_Implementation(const FSPHealthActionData& ActionData) override;

	// 유닛의 체력 변화를 시각적으로 표현한다. (UI, 이펙트 등)
	void ApplyVisualDamage_Implementation(int32 DisplayAmount, int32 NewTargetHealth, bool bIsCritical, const FGameplayTagContainer& TypeTags) override;

	// 유닛의 현재 체력을 반환한다.
	int32 GetCurrentHealth_Implementation() const override;





	UFUNCTION(Server, Reliable,BlueprintCallable)
	void ServerExecuteSkill(const FGameplayTag& SkillSlotTag, const FSkillTargetData& Target);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerRequestMove(const FIntPoint& Destination);

	UFUNCTION()
	void OnRep_GridPosition(FIntPoint OldGridPosition);

	UFUNCTION()
	void OnRep_UnitData();

	UFUNCTION()
	void OnRep_Faction();

	// == Presentation ==

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void PresentDeath();

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void PresentRevive();

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void PresentHit();

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void PresentMoveSegment(const FIntPoint& From, const FIntPoint& To);

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void NotifyMyPresentationFinished();





	// == Getter / Setter ==

	void SetCurrentSlot(UUnitSlot* NewSlot) { CurrentSlot = NewSlot; }
	void SetGridPosition(const FIntPoint& NewPosition) { GridPosition = NewPosition; }
	void SetFaction(EFaction NewFaction);
	
	UUnitSlot* GetCurrentSlot() const { return CurrentSlot; }
	EFaction GetFaction() const { return Faction; }
	FIntPoint GetGridPosition() const { return GridPosition; }
	int32 GetSpeed() const;
	UStatComponent* GetStatComponent() const { return StatComponent; }
	USkillComponent* GetSkillComponent() const { return SkillComponent; }
	UStateComponent* GetStateComponent() const { return StateComponent; }
	UGridMoveComponent* GetGridMoveComponent() const { return GridMoveComponent; }
	FGameplayTagContainer GetStateTags() const;
	const UUnitAnimSetDataAsset* GetAnimSet() const;

	UFUNCTION(BlueprintCallable, Category = "Unit")
	UCombatEventComponent* GetCombatEventComponent() const;

	/** 마우스 호버 상태를 알린다. (플레이어 컨트롤러 / BP에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Unit|UI")
	void SetHealthBarHovered(bool bHovered);

	/** 선택 상태를 알린다. (플레이어 컨트롤러 / BP에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Unit|UI")
	void SetHealthBarSelected(bool bSelected);


private:
	
	UUnitHealthBarWidget* GetHealthBarWidget() const;

	// 체력바 위젯 클래스/높이/초기값 갱신
	void RefreshHealthBar();

	// 체력바 높이 계산
	float CalculateHealthBarHeight() const;

	EHealthBarReason HealthBarReasons = EHealthBarReason::None;
	FTimerHandle RecentHitTimerHandle;

	/** 사건성 이유 하나를 켜거나 끈다. 실제로 바뀐 경우에만 표시를 갱신한다. */
	void SetHealthBarReason(EHealthBarReason Reason, bool bEnable);

	/** 정책 + 연출 체력 + 사건 이유를 종합해 체력바 표시 여부를 결정한다. */
	void UpdateHealthBarVisibility();

	/** RecentHit 이유를 해제한다. 타이머 콜백. */
	void ClearRecentHitReason();

	/** 피격 후 체력바를 붙잡아 둘 시간(초). 연출이 끝나기 전에 사라지지 않게 한다. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float RecentHitHoldSeconds = 2.0f;

};

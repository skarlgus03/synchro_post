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
class UUnitHealthBarWidget;
class UUnitAnimSetDataAsset;
class UUnitHealthBarComponent;


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
	TObjectPtr<UUnitHealthBarComponent> HealthBarWidgetComponent;



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

	// 액터를 이 타일 위에 세운다. 순수 배치 함수
	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SnapToTile(const FIntPoint& TileCoord);

	/* 이 유닛이 해당 타일에 서 있을 때의 액터 위치
	*  캡슐 중심이 액터 원점이므로 타일 바닥 + CapusleHalfHeight 임
	*  이동 보간의 목표 위치도 이것을 써야함.
	*/
	UFUNCTION(BlueprintCallable, Category = "Unit")
	FVector GetStandLocation(const FIntPoint& Coord) const;

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
	void SetHovered(bool bHovered);

	/** 선택 상태를 알린다. (플레이어 컨트롤러 / BP에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Unit|UI")
	void SetSelected(bool bSelected);

	UFUNCTION(BlueprintCallable, Category = "Unit|UI")
	bool IsHovered() const { return bIsHovered; }

	UFUNCTION(BlueprintCallable, Category = "Unit|UI")
	bool IsSelected() const { return bIsSelected; }




private:
	
	bool bIsHovered = false;
	bool bIsSelected = false;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Types/SPGameplayTags.h"
#include "Types/SynchroPostTypes.h"
#include "Unit.generated.h"

class UUnitDataAsset;
class UUnitSlot;
class USkillComponent;
class UStatComponent;
class UStateComponent;
class UCombatEventComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDied, AUnit*, DeadUnit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitRevived, AUnit*, RevivedUnit);

UCLASS()
class SYNCHROPOST_API AUnit : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AUnit();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:

	// 에디터에서 기본적으로 사용할 DA를 지정할 수 있는 프로퍼티
	UPROPERTY(EditAnywhere, Category = "Unit Data")
	TObjectPtr<UUnitDataAsset> DefaultUnitData;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UUnitSlot> CurrentSlot;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<USkillComponent> SkillComponent;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UStatComponent> StatComponent;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UStateComponent> StateComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	EFaction Faction = EFaction::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Unit")
	bool bIsDead = false;

	// Current grid position of the unit on the map
	UPROPERTY(EditAnywhere, Category = "Unit State|Grid")
	FIntPoint GridPosition;

public:

	// 게임 도중에 실시간으로 바뀔 수 있는 DA 프로퍼티
	UPROPERTY(BlueprintReadOnly, Category = "Unit Data")
	TObjectPtr<const UUnitDataAsset> CurrentUnitData;

	

	FOnUnitDied OnUnitDied;

	FOnUnitRevived OnUnitRevived;

public:

	bool IsDead() const { return bIsDead; }

	void InitializeUnit(const UUnitDataAsset* UnitData);

	UFUNCTION()
	void HandleHealthChanged(int32 NewHealth, const FSPDamageData& DamageData);

	UFUNCTION(BlueprintCallable, Category = "Unit")
	int32 ApplyDamage(FSPDamageData DamageData);

	UFUNCTION(Server, Reliable)
	void ServerExecuteSkill(const FGameplayTag& SkillSlotTag, const FSkillTargetData& Target);


	// == Getter / Setter ==

	void SetCurrentSlot(UUnitSlot* NewSlot) { CurrentSlot = NewSlot; }
	UUnitSlot* GetCurrentSlot() const { return CurrentSlot; }
	void SetGridPosition(const FIntPoint& NewPosition) { GridPosition = NewPosition; }
	EFaction GetFaction() const { return Faction; }
	FIntPoint GetGridPosition() const { return GridPosition; }
	int32 GetSpeed() const;
	UStatComponent* GetStatComponent() const { return StatComponent; }

	UFUNCTION(BlueprintCallable, Category = "Unit")
	UCombatEventComponent* GetCombatEventComponent() const;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Types/SPGameplayTags.h"
#include "Unit.generated.h"

class UUnitDataAsset;
class UUnitSlot;
class USkillComponent;
class UUnitStatComponent;
class UStateComponent;

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

	UPROPERTY()
	TObjectPtr<UUnitSlot> CurrentSlot;

	UPROPERTY()
	TObjectPtr<USkillComponent> SkillComponent;

	UPROPERTY()
	TObjectPtr<UUnitStatComponent> StatComponent;

	UPROPERTY()
	TObjectPtr<UStateComponent> StateComponent;

public:

	// 게임 도중에 실시간으로 바뀔 수 있는 DA 프로퍼티
	UPROPERTY(BlueprintReadOnly, Category = "Unit Data")
	TObjectPtr<const UUnitDataAsset> CurrentUnitData;

	// Current grid position of the unit on the map
	UPROPERTY(EditAnywhere, Category = "Unit State|Grid")
	FIntPoint GridPosition;


	// 나중에 스킬 컴포넌트에 넣을 것.
	int32 ActionPoints;

	int32 BehaviorPoints;
public:

	void InitializeUnit(const UUnitDataAsset* UnitData);

	void SetCurrentSlot(UUnitSlot* NewSlot) { CurrentSlot = NewSlot; }

	UUnitSlot* GetCurrentSlot() const { return CurrentSlot; }
};

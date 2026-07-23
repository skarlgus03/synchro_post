#include "Unit/Unit.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Unit/UnitDataAsset.h"
#include "Unit/SkillComponent.h"
#include "Unit/StatComponent.h"
#include "Unit/StateComponent.h"

// Sets default values
AUnit::AUnit()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	if (UCharacterMovementComponent* CharacterMoveComp = GetCharacterMovement())
	{
		CharacterMoveComp->GravityScale = 0.0f;
		CharacterMoveComp->bComponentShouldUpdatePhysicsVolume = false;
	}

	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;

	SkillComponent = CreateDefaultSubobject<USkillComponent>(TEXT("SkillComponent"));
	StatComponent = CreateDefaultSubobject<UStatComponent>(TEXT("StatComponent"));
	StateComponent = CreateDefaultSubobject<UStateComponent>(TEXT("StateComponent"));

}

// Called when the game starts or when spawned
void AUnit::BeginPlay()
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] BeginPlay Start - StatComponent: %s"), *GetName(), StatComponent ? TEXT("Valid") : TEXT("NULL"));

	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[%s] Super::BeginPlay After - StatComponent: %s"), *GetName(), StatComponent ? TEXT("Valid") : TEXT("NULL"));

	InitializeUnit(nullptr);

	UE_LOG(LogTemp, Warning, TEXT("[%s] InitializeUnit After - StatComponent: %s"), *GetName(), StatComponent ? TEXT("Valid") : TEXT("NULL"));

	if (!StatComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] StatComponent is NULL in BeginPlay!"), *GetName());
		return;
	}

	StatComponent->OnHealthChanged.AddDynamic(this, &AUnit::HandleHealthChanged);
}


// Called every frame
void AUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AUnit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


void AUnit::InitializeUnit(const UUnitDataAsset* UnitData)
{
	if (UnitData)
	{
		CurrentUnitData = UnitData;
	}
	else
	{
		CurrentUnitData = DefaultUnitData;
	}

	if (CurrentUnitData->UnitMesh.IsValid())
	{
		GetMesh()->SetSkeletalMesh(CurrentUnitData->UnitMesh.LoadSynchronous());
	}

	if (SkillComponent)
	{
		SkillComponent->InitializeSkillComponent(CurrentUnitData);
	}

	if (StatComponent)
	{
		StatComponent->InitializeStats(CurrentUnitData->UnitStatData, 1);
	}

}

void AUnit::HandleHealthChanged(int32 NewHealth, int32 DamageAmount, const FGameplayTagContainer& DamageTypeTags)
{

	const bool bWasDead = bIsDead;
	bIsDead = (NewHealth <= 0);

	// 상태가 죽음으로 바뀌었거나 회복된 경우
	if (bIsDead && !bWasDead)
	{
		// 유닛 사망 이벤트 발생
		OnUnitDied.Broadcast(this);
	}
	else if (!bIsDead && bWasDead)
	{
		// 유닛 부활 이벤트 발생
	}
}

int32 AUnit::ApplyDamage(FSPDamageData DamageData)
{
	if (!StatComponent)
	{
		return 0;
	}
	if (bIsDead)
	{
		return 0;
	}

	return StatComponent->ApplyDamage(DamageData);
}

int32 AUnit::GetSpeed() const
{
	if (StatComponent)
	{
		return StatComponent->GetStat(SPTags::Stat::Combat::Primary::Speed);
	}
	return 0;
}



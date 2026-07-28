#include "Unit/Unit.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Unit/UnitDataAsset.h"
#include "Unit/SkillComponent.h"
#include "Unit/StatComponent.h"
#include "Unit/StateComponent.h"
#include "Framework/CombatEventComponent.h"
#include "Framework/SPGameState.h"
#include "Types/SPCombatEventStructure.h"


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
	
	Super::BeginPlay();

	InitializeUnit(nullptr);

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

void AUnit::HandleHealthChanged(int32 NewHealth, const FSPDamageData& DamageData)
{

	const bool bWasDead = bIsDead;
	bIsDead = (NewHealth <= 0);

	// 상태가 죽음으로 바뀌었거나 회복된 경우
	if (bIsDead && !bWasDead)
	{
		OnUnitDied.Broadcast(this);

		if (UCombatEventComponent* EventComp = GetCombatEventComponent())
		{
			FCombatEvent Event;
			Event.Source = this;
			FUnitDiedPayload DiedPayload;
			DiedPayload.Causer = Cast<AUnit>(DamageData.DamageCauser);
			DiedPayload.DeathCoordinate = GetGridPosition();
			Event.Payload = FInstancedStruct::Make(DiedPayload);

			EventComp->PushEvent(Event);
		}
	}
	else if (!bIsDead && bWasDead)
	{
		// 유닛 부활 이벤트 발생
		OnUnitRevived.Broadcast(this);

		if (UCombatEventComponent* EventComp = GetCombatEventComponent())
		{
			FCombatEvent Event;
			Event.Source = this;

			FUnitRevivedPayload RevivedPayload;
			RevivedPayload.Causer = Cast<AUnit>(DamageData.DamageCauser);
			
			// 만약 살릴위치가 다른곳이라면 이쪽 코드 수정해줘야한다.
			// 일단 그냥 죽은 유닛 위치를 넣었다.
			RevivedPayload.RevivalCoordinate = GetGridPosition();
			Event.Payload = FInstancedStruct::Make(RevivedPayload);

			EventComp->PushEvent(Event);
		}
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

void AUnit::ServerExecuteSkill_Implementation(const FGameplayTag& SkillSlotTag, const FSkillTargetData& Target)
{
	if (SkillComponent)
	{
		SkillComponent->ExecuteSkill(SkillSlotTag, Target);
	}
}

void AUnit::NotifyMyPresentationFinished()
{
	if (UCombatEventComponent* EventComp = GetCombatEventComponent())
	{
		EventComp->NotifyPresentationFinished();
	}
}

int32 AUnit::GetSpeed() const
{
	if (StatComponent)
	{
		return StatComponent->GetStat(SPTags::Stat::Combat::Primary::Speed);
	}
	return 0;
}

UCombatEventComponent* AUnit::GetCombatEventComponent() const
{
	if (ASPGameState* GameState = GetWorld()->GetGameState<ASPGameState>())
	{
		return GameState->GetCombatEventComponent();
	}

	return nullptr;
}


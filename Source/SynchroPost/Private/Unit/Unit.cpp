#include "Unit/Unit.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Unit/UnitDataAsset.h"

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

	bReliplicates = true;
}

// Called when the game starts or when spawned
void AUnit::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeUnit(nullptr);
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

	// 나머지 컴포넌트가 만들어지면, 초기화하기.
}





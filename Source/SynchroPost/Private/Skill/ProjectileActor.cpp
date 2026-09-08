
#include "Skill/ProjectileActor.h"
#include "Interface/Damageable.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Unit/Unit.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileActor::AProjectileActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	TravelEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TravelEffect"));
	TravelEffectComponent->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

void AProjectileActor::Initialize(const FCombatEventTarget& InTargetData, const FVector& DestinationLocation, TSoftObjectPtr<UNiagaraSystem> InImpactEffect,AUnit* InOwningCaster, float Speed)
{
	CachedTargetData = InTargetData;
	ImpactEffect = InImpactEffect;

	const FVector Direction = (DestinationLocation - GetActorLocation()).GetSafeNormal();
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->Velocity = Direction * Speed;

	const float Distance = FVector::Dist(GetActorLocation(), DestinationLocation);
	const float TravelDuration = Speed > 0.f ? Distance / Speed : 0.f;

	OwningCaster = InOwningCaster;

	GetWorldTimerManager().SetTimer(ArrivalTimerHandle, this, &AProjectileActor::OnArrival, TravelDuration, false);
}

void AProjectileActor::OnArrival()
{
	if (UNiagaraSystem* Effect = ImpactEffect.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect, GetActorLocation());
	}

	if (AActor* Target = CachedTargetData.Target.Get())
	{
		if (Target->Implements<UDamageable>())
		{
			const int32 DisplayAmount = CachedTargetData.HealthBeforeChange - CachedTargetData.HealthAfterChange;
			IDamageable::Execute_ApplyVisualDamage(Target, DisplayAmount, CachedTargetData.HealthAfterChange,
				CachedTargetData.ActionData.bIsCriticalHit, CachedTargetData.ActionData.ActionTypeTags);
		}
	}
	
	Destroy();
}



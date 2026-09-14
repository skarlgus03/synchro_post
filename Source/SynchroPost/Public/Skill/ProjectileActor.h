#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/SynchroPostTypes.h"
#include "ProjectileActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class AUnit;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnProjectileArrived, const FCombatEventTarget& /*Result*/);

UCLASS()
class SYNCHROPOST_API AProjectileActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectileActor();

	void Initialize(const FCombatEventTarget& InTargetData, const FVector& DestinationLocation, TSoftObjectPtr<UNiagaraSystem> InImpactEffect, AUnit* InOwningCaster, float Speed = 2000.f);

	// 도착시 발생. Step이 구독해서 결과를 표시 + 도착 카운트를 한다.
	FOnProjectileArrived OnArrived;
protected:

	UFUNCTION()
	void OnArrival();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UNiagaraComponent> TravelEffectComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
		
	FCombatEventTarget CachedTargetData;
	TSoftObjectPtr<UNiagaraSystem> ImpactEffect;
	FTimerHandle ArrivalTimerHandle;
	TWeakObjectPtr<AUnit> OwningCaster;
};
